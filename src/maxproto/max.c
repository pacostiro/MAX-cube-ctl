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

#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "maxmsg.h"
#include "max.h"
#include "base64.h"

static void nullifyCommas(char *start, char* end)
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

static int parseMAXData(char *MAXData, int size, MAX_msg_list** msg_list)
{
    char *pos = MAXData, *tmp;
    char *end = MAXData + size - 1;
    MAX_msg_list *new = NULL, *iter;
    int len;
    size_t outlen, off;

    if (MAXData == NULL)
    {
        errno = EINVAL;
        return -1;
    }
    while (pos != NULL && pos < end)
    {
        if (*(pos + 1) != ':')
        {
            errno = EBADMSG;
            return -1;
        }
        tmp = strstr(pos, "\r\n");
        if (tmp == NULL)
        {
            errno = EBADMSG;
            return -1;
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
            new->next = NULL;
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
                new->MAX_msg = (struct MAX_message*)base64_to_hex(pos + off,
                               len, off, 0, &outlen);

                memcpy(new->MAX_msg, pos, off);
                break;
            case 'L':
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
    return 0;
}

int MAXDiscoverSend()
{
    return 0;
}

int MAXDiscoverRecv(struct sockaddr *sa, size_t sa_len)
{
    return 0;
}

int MAXConnect(struct sockaddr *sa)
{
    int sockfd;

    if((sockfd = socket(sa->sa_family, SOCK_STREAM, 0)) < 0)
    {
        return -1;
    }

    if(connect(sockfd, sa, sizeof(*sa)) < 0)
    {
        return -1;
    }

    return sockfd;
}

int MAXDisconnect(int connectionId)
{
    return close(connectionId);
}

int MAXMsgSend(int connectionId, MAX_msg_list *output_msg_list)
{
    return 0;
}

int MaxMsgRecv(int connectionId, MAX_msg_list **input_msg_list, int tmo)
{
#ifdef __CYGWIN__
    fd_set fds;
#endif
    char recvBuff[4096];
    struct timeval tv;
    int n;

    tv.tv_sec = tmo;
    tv.tv_usec = 0;
    
#ifdef __CYGWIN__
    FD_ZERO(&fds);
    FD_SET(connectionId, &fds);

    do {
        n = select(connectionId + 1, &fds, NULL, NULL, &tv);
        if (n == -1)
        {
            return -1;
        }
        else if (n > 0)
        {
            n = read(connectionId, recvBuff, sizeof(recvBuff) - 1);
            parseMAXData(recvBuff, n, input_msg_list);
        }
    } while (n > 0);
#else
    if (setsockopt(connectionId, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
        sizeof(struct timeval)) < 0)
    {
        return -1;
    }

    while ((n = read(connectionId, recvBuff, sizeof(recvBuff) - 1)) > 0)
    {
        parseMAXData(recvBuff, n, input_msg_list);
    }
#endif

    return 0;
}

