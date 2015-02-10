/* Copyright (c) 2015, Costin Popescu
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "maxmsg.h"

#define MSG_END "\r\n"
#define MAX_SCHED 13

typedef struct MME
{
    struct MME *prev;
    struct MME *next;
    struct MAX_message *MAX_msg;
} MAX_msg_list;

enum MaxDeviceType
{
    Cube = 0,
    RadiatorThermostat = 1,
    RadiatorThermostatPlus = 2,
    WallThermostat = 3,
    ShutterContact = 4,
    EcoButton = 5
};

static char* device_types[] = {
    "Cube",
    "RadiatorThermostat",
    "RadiatorThermostatPlus",
    "WallThermostat",
    "ShutterContact",
    "EcoButton"};

static char* week_days[] = {
    "Saturday",
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday"};

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

void build_decoding_table() {
    int i;
    decoding_table = malloc(256);

    for (i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}

char *base64_encode(const unsigned char *data,
        size_t input_length,
        size_t output_off,
        size_t output_pad,
        size_t *output_length) {
    int i, j;
    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = malloc(*output_length + output_off + output_pad);
    if (encoded_data == NULL) return NULL;

    for (i = 0, j = output_off; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i + output_off] = '=';

    return encoded_data;
}

unsigned char *base64_decode(const char *data,
        size_t input_length,
        size_t output_off,
        size_t output_pad,
        size_t *output_length) {
    unsigned char *decoded_data;
    int i, j;

    if (decoding_table == NULL) build_decoding_table();

    if (input_length % 4 != 0)
    {
        *output_length = 0;
        printf("base64_decode error\n");
        return NULL;
    }

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    decoded_data = malloc(*output_length + output_off + output_pad);
    if (decoded_data == NULL) return NULL;

    for (i = 0, j = output_off; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
            + (sextet_b << 2 * 6)
            + (sextet_c << 1 * 6)
            + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}

void base64_cleanup() {
    free(decoding_table);
}

void nullifyCommas(char *start, char* end)
{
    if (start == NULL || end == NULL)
        return;
    while(start < end)
    {
        if (*start == ',')
            *start = '\0';
        start++;
    }
    *end = '\0';
}

int parseMAXData(char *MAXData, int size, MAX_msg_list** msg_list)
{
    char *pos = MAXData, *tmp;
    char *end = MAXData + size - 1;
    MAX_msg_list *new = NULL, *iter;
    int done = 0, len;
    size_t outlen, off;

    if (MAXData == NULL)
    {
        return -1;
    }
    while (pos != NULL && pos < end)
    {
        if (*(pos + 1) != ':')
        {
            printf("Warning: illegal message\n");
            break;
        }
        tmp = strstr(pos, "\r\n");
        if (tmp == NULL)
        {
            printf("Warning: illegal message\n");
            break;
        }
        nullifyCommas(pos, tmp);
        tmp += 2;
        new = (MAX_msg_list*)malloc(sizeof(MAX_msg_list));
        if (*msg_list == NULL)
        {
            *msg_list = new;
            new->prev = NULL;
            new->next = NULL;
        }
        else
        {
            iter = *msg_list;
            while (iter->next != NULL) {
                iter = iter->next;
            }
            iter->next = new;
            new->prev = iter;
        }
        switch (*pos)
        {
            case 'H':
            {
                int H_len = sizeof(struct MAX_message) - 1 +
                            sizeof(struct H_Data);
                new->MAX_msg = malloc(H_len);
                memcpy(new->MAX_msg, pos, H_len);
                break;
            }
            case 'C':
                /* Calculate offset of second field (C_Data_Device)*/
                off = sizeof(struct MAX_message) - 1 + sizeof(struct C_Data);
                /* Move to second field */
                /* Calculate length of second field */
                len = tmp - 2 - pos - off;
                new->MAX_msg = (struct MAX_message*)base64_decode(pos + off,
                               len, off, 0, &outlen);

                memcpy(new->MAX_msg, pos, off);
                break;
            case 'L':
                done = 1;
            case 'M':
            case 'S':
            case 'Q':
            default:
                new->MAX_msg = malloc(tmp - pos);
                memcpy(new->MAX_msg, pos, tmp - pos);
                break;
        }
        pos = tmp;
    }
    return done;
}

void dumpMAXpkt(MAX_msg_list* msg_list)
{
    while (msg_list != NULL) {
        char* md = msg_list->MAX_msg->data;
        printf("Message type %c\n", msg_list->MAX_msg->type);
        switch (msg_list->MAX_msg->type)
        {
            case 'H':
                {
                    int year, month, day;
                    int hour, minutes;
                    char tmp[16];
                    struct H_Data *H_D = (struct H_Data*)md;
                    printf("\tSerial no           %s\n",
                            H_D->Serial_number);
                    printf("\tRF address          %s\n",
                            H_D->RF_address);
                    printf("\tFirmware version    %s\n",
                            H_D->Firmware_version);
                    printf("\tunknown             %s\n",
                            H_D->unknown);
                    printf("\tHTTP connection id  %s\n",
                            H_D->HTTP_connection_id);
                    printf("\tDuty cycle          %s\n",
                            H_D->Duty_cycle);
                    printf("\tFree Memory Slots   %d\n",
                            (int) strtol(H_D->Free_Memory_Slots, NULL, 16));
                    memcpy(tmp, H_D->Cube_date, 2);
                    tmp[2] = '\0';
                    year = 2000 + strtol(tmp, NULL, 16);
                    memcpy(tmp, H_D->Cube_date + 2, 2);
                    tmp[2] = '\0';
                    month = strtol(tmp, NULL, 16);
                    memcpy(tmp, H_D->Cube_date + 4, 2);
                    tmp[2] = '\0';
                    day = strtol(tmp, NULL, 16);
                    printf("\tCube date           %d/%d/%d\n",
                            day, month, year);
                    memcpy(tmp, H_D->Cube_time, 2);
                    tmp[2] = '\0';
                    hour = strtol(tmp, NULL, 16);
                    memcpy(tmp, H_D->Cube_time + 2, 2);
                    tmp[2] = '\0';
                    minutes = strtol(tmp, NULL, 16);
                    printf("\tCube time           %02d:%02d\n", hour, minutes);
                    printf("\tState Cube Time     %s\n", H_D->State_Cube_Time);
                    printf("\tNTP Counter         %s\n", H_D->NTP_Counter);
                    break;
                }
            case 'C':
                {
                    struct C_Data *C_D = (struct C_Data*)md;
                    unsigned char *tmp;
                    char buf[16], *str;
                    int val;
                    union C_Data_Device *data =
                        (union C_Data_Device*)(md + sizeof(struct C_Data));

                    switch (data->device.Device_Type[0])
                    {
                        case RadiatorThermostat:
                        {
                            float fval;
                            int day, hours, mins;
                            uint16_t ws;
                            int s;
                            union C_Data_Config *config =
                                (union C_Data_Config*)((char*)data +
                                        sizeof(union C_Data_Device));

                            printf("\tRF address          %s\n", C_D->RF_address);

                            val = data->device.Data_Length[0];
                            printf("\tData Length         %d\n", val);

                            tmp = data->device.Address_of_device;
                            printf("\tAddress_of_device   %x%x%x\n", tmp[0], tmp[1], tmp[2]);

                            val = data->device.Device_Type[0];
                            printf("\tDevice Type         %s\n", device_types[val]);

                            val = data->device.Room_ID[0];
                            printf("\tRoom ID             %d\n", val);

                            val = data->device.Firmware_version[0];
                            printf("\tFirmware version    %d\n", val);

                            val = data->device.Test_Result[0];
                            printf("\tTest Result         %d\n", val);

                            strncpy(buf, data->device.Serial_Number,
                                    sizeof(data->device.Serial_Number));
                            buf[sizeof(data->device.Serial_Number)] = '\0';
                            printf("\tSerial Number       %s\n", buf);

                            fval = config->rtc.Comfort_Temperature[0] / 2.;
                            printf("\tComfort Temperature %.1f\n", fval);
                            fval = config->rtc.Eco_Temperature[0] / 2.;
                            printf("\tEco Temperature     %.1f\n", fval);
                            fval = config->rtc.Temperature_offset[0] / 2. - 3.5;
                            printf("\tTemperature offset  %.1f\n", fval);
                            day = config->rtc.Decalcification[0] >> 5;
                            hours = (config->rtc.Decalcification[0] & 0b11111);
                            printf("\tDecalcification     %s %2d:00\n", week_days[day], hours);
                            memcpy(&ws, &config->rtc.Weekly_Program[0], sizeof(ws));
                            s = 0;
                            day = 0;
                            printf("\tWeekly Program\n");
                            while (s < sizeof(config->rtc.Weekly_Program))
                            {
                                fval = (config->rtc.Weekly_Program[s] >> 1) / 2.;
                                ws = (((config->rtc.Weekly_Program[s] & 1) << 8) |
                                        config->rtc.Weekly_Program[s + 1]) * 5;
                                hours = ws / 60;
                                mins = ws %60;
                                printf("\t\t%-10s  %.1f until %02d:%02d\n", week_days[day],
                                        fval, hours, mins);
                                if (hours >= 24)
                                {
                                    s = (s / (MAX_SCHED * 2) + 1) * MAX_SCHED * 2;
                                    day++;
                                }
                                else
                                {
                                    s += 2;
                                }
                            }
                            break;
                        }
                        case Cube:
                        {
                            union C_Data_Config *config =
                                (union C_Data_Config*)((char*)data +
                                        sizeof(union C_Data_Device));

                            printf("\tRF address          %s\n",
                                C_D->RF_address);

                            val = data->cube.Data_Length[0];
                            printf("\tData Length         %d\n", val);

                            tmp = data->cube.Address_of_device;
                            printf("\tAddress_of_device   %x%x%x\n", tmp[0],
                                tmp[1], tmp[2]);

                            val = data->cube.Device_Type[0];
                            printf("\tDevice Type         %s\n",
                                device_types[val]);

                            val = data->cube.Firmware_version[0];
                            printf("\tFirmware version    %d\n", val);

                            strncpy(buf, data->cube.Serial_Number,
                                    sizeof(data->cube.Serial_Number));
                            buf[sizeof(data->cube.Serial_Number)] = '\0';
                            printf("\tSerial Number       %s\n", buf);
                            
                            val = config->cubec.Is_Portal_Enabled[0];
                            str = config->cubec.Portal_URL;
                            printf("\tPortal              %s\n",
                                (val == 0) ? "disabled" : str);
                            
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
            default:
                break;
        }
        msg_list = msg_list->next;
    }
}

struct MAX_message* create_s_cmd(int *len)
{
    struct s_Data s_Data;
    struct MAX_message *m_s;
    size_t outlen, off;
    int temp, hours, minutes, t;

    s_Data.Base_String[0] = 0x00;
    s_Data.Base_String[1] = 0x04;
    s_Data.Base_String[2] = 0x10;
    s_Data.Base_String[3] = 0x00;
    s_Data.Base_String[4] = 0x00;
    s_Data.Base_String[5] = 0x00;

    s_Data.RF_Address[0] = 0x11;
    s_Data.RF_Address[1] = 0x32;
    s_Data.RF_Address[2] = 0xb5;

    s_Data.Room_Nr[0] = 0x02;
    s_Data.Day_of_week[0] = 1;

    temp = 20, hours = 6, minutes = 30;
    t = (60 * hours + minutes) / 5;
    s_Data.Temp_and_Time[0] = (((temp * 2) << 1) | ((t >> 8) & (0x1)));
    s_Data.Temp_and_Time[1] = (t & 0xff);

    temp = 22, hours = 24, minutes = 00;
    t = (60 * hours + minutes) / 5;
    s_Data.Temp_and_Time2[0] = (((temp * 2) << 1) | ((t >> 8) & (0x1)));
    s_Data.Temp_and_Time2[1] = (t & 0xff);

    s_Data.Temp_and_Time3[0] = 0;
    s_Data.Temp_and_Time3[1] = 0;

    s_Data.Temp_and_Time4[0] = 0;
    s_Data.Temp_and_Time4[1] = 0;

    s_Data.Temp_and_Time5[0] = 0;
    s_Data.Temp_and_Time5[1] = 0;

    s_Data.Temp_and_Time6[0] = 0;
    s_Data.Temp_and_Time6[1] = 0;

    s_Data.Temp_and_Time7[0] = 0;
    s_Data.Temp_and_Time7[1] = 0;

    off = sizeof(struct MAX_message) - 1;
    m_s = (struct MAX_message*)base64_encode((const unsigned char*)&s_Data, sizeof(s_Data), off,
            strlen(MSG_END) + 1, &outlen);
    m_s->type = 's';
    m_s->colon = ':';
    strcpy(&m_s->data[off + outlen], MSG_END);
    printf("Message s\n%s\n off:%d outlen: %d", (char*)m_s, (int)off, (int)outlen);

    *len = off + outlen + strlen(MSG_END);

    return m_s;
}

int main(int argc, char *argv[])
{
    int sockfd = 0, n = 0;
    char recvBuff[4096];
    struct sockaddr_in serv_addr;
    MAX_msg_list* msg_list = NULL;
    struct MAX_message *m_s, *m_l;
    struct l_Data *l_d;

    if(argc < 3 || argc > 4)
    {
        printf("\n Usage: %s <ip of MAX! cube> <port pf MSX! cube> [set]\n",argv[0]);
        return 1;
    }

    printf("Welcome MAX! cube\n");

    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2])); 

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
        return 1;
    }

    if (argc == 4)
        goto set;

/*
get:
*/
    while ((n = read(sockfd, recvBuff, sizeof(recvBuff) - 1)) > 0)
    {
        int done;
        printf("Received %d bytes from MAX! cube\n", n);
        done = parseMAXData(recvBuff, n, &msg_list);
        if (done)
        {
            break;
        }
    }

    dumpMAXpkt(msg_list);

    return 0;

set:
    m_s = create_s_cmd(&n);
    write(sockfd, m_s, n);
    free(m_s);
    m_l = malloc(sizeof(struct MAX_message) - 1 + sizeof(struct l_Data));
    m_l->type = 'l';
    m_l->colon = ':';
    l_d = (struct l_Data*)m_l->data;
    memcpy(l_d, MSG_END, sizeof(MSG_END));
    write(sockfd, &m_l, sizeof(m_l));
    free(m_l);
    return 0;
}

