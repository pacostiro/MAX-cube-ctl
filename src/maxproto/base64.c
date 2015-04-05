/* Copyright (c) 2015, Costin Popescu
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "base64.h"

/* Function that transforms 6 bit values to ASCII char */
static char base64_index_table[] =
    {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
     'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
     'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
     'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
     '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

/* This is the inverse function of base64_index_table */
static char *inv_base64_index_table = NULL;

void create_inv_base64_index_table()
{
    int i;
    inv_base64_index_table = malloc(256);

    for (i = 0; i < 64; i++)
    {
        inv_base64_index_table[(unsigned char) base64_index_table[i]] = i;
    }
}

void free_inv_base64_index_table()
{
    free(inv_base64_index_table);
}

char *hex_to_base64(const unsigned char *data, size_t data_sz,
        size_t output_off, size_t output_pad, size_t *output_sz)
{
    int i, j;
    char *base64_text;

    /* Compute final size of output */
    *output_sz = 4 * ((data_sz + 2) / 3);
    base64_text = malloc(*output_sz + output_off + output_pad);

    if (base64_text == NULL)
    {
        output_sz = 0;
        return NULL;
    }

    for (i = 0, j = output_off; i < data_sz;)
    {
        uint32_t B0, B1, B2, tmp;
        
        /* Get next 3 bytes (24 bits) to split in 4 groups of 6 bits */
        B0 = (i < data_sz) ? (unsigned char)data[i++] : 0;
        B1 = (i < data_sz) ? (unsigned char)data[i++] : 0;
        B2 = (i < data_sz) ? (unsigned char)data[i++] : 0;
        
        /* Unify all 24 bits into an integer */
        tmp = (B0 << 16) + (B1 << 8) + B2;

        /* Take every 6 bits and store into one element of output array */
        base64_text[j++] = base64_index_table[(tmp >> 18) & 0b00111111];
        base64_text[j++] = base64_index_table[(tmp >> 12) & 0b00111111];
        base64_text[j++] = base64_index_table[(tmp >> 6) & 0b00111111];
        base64_text[j++] = base64_index_table[tmp & 0b00111111];
    }

    /* Add padding with '=' to get a length divisible by 3 */
    j = data_sz % 3;
    if (j != 0)
    {
        j = 3 - j;
        for (i = 0; i < j; i++)
        {
            base64_text[*output_sz - 1 - i + output_off] = '=';
        }
    }

    return base64_text;
}

unsigned char *base64_to_hex(const char *data, size_t data_sz,
        size_t output_off, size_t output_pad, size_t *output_sz)
{
    unsigned char *hex_data;
    int i, j;
    size_t pad_off;

    /* Initialize inverse function which transforms base64 chars to 6 bit
     * values. */
    if (inv_base64_index_table == NULL)
    {
        create_inv_base64_index_table();
    }

    /* Length of input data has to be divisible by 4 */
    if (data_sz % 4 != 0)
    {
        *output_sz = 0;
        printf("base64_to_hex error: Bad input length!\n");
        return NULL;
    }

    /* Compute final size of output data */
    *output_sz = data_sz / 4 * 3;
    /* Adjust according to padding at the end. One '=' symbol means one byte
     * less */
    if (data[data_sz - 1] == '=')
    {
        (*output_sz)--;
    }
    if (data[data_sz - 2] == '=')
    {
        (*output_sz)--;
    }

    hex_data = malloc(*output_sz + output_off + output_pad);
    /* Calculate padding offset */
    pad_off = output_off + (*output_sz);
    if (hex_data == NULL)
    {
        *output_sz = 0;
        return NULL;
    }

    for (i = 0, j = output_off; i < data_sz;)
    {
        uint32_t tmp = 0, k;
        /* Group 4 elements containing 6 bits values to be split into 3 bytes */
        for (k = 0; k < 4; k++)
        {
            tmp = (tmp << 6);
            tmp |= (data[i] == '=') ? 0 : inv_base64_index_table[(int)data[i]];
            i++;
        }

        /* Split 24 bits into 3 bytes */
        if (j < pad_off)
        {
            hex_data[j++] = (tmp >> 16) & 0xFF;
        }
        if (j < pad_off)
        {
            hex_data[j++] = (tmp >> 8) & 0xFF;
        }
        if (j < pad_off)
        {
            hex_data[j++] = tmp & 0xFF;
        }
    }

    return hex_data;
}


