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

#include "max.h"
#include "maxmsg.h"
#include "base64.h"

#include "max_parser.h"

#if 1
#define MAX_DEBUG
#endif

#define MAX_CONFIG_FILE "MAX.conf"

#define MSG_END "\r\n" /* Message terminator sequence */
#define MSG_TMO 2      /* Message receive timeout */

enum Mode
{
    Auto = 0,
    Comfort = 1,
    Eco = 2
};

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
    m_s = (struct MAX_message*)hex_to_base64((const unsigned char*)&s_Data, sizeof(s_Data), off,
            strlen(MSG_END) + 1, &outlen);
    m_s->type = 's';
    m_s->colon = ':';
    strcpy(&m_s->data[off + outlen], MSG_END);
    printf("Message s\n%s\n off:%d outlen: %d", (char*)m_s, (int)off, (int)outlen);

    *len = off + outlen + strlen(MSG_END);

    return m_s;
}

void help(const char* program)
{
    printf("Usage: %s <ip of MAX! cube> <port pf MSX! cube> <command> <params>\n", program);
    printf("\tCommands  Params\n" \
           "\tget       status\n" \
           "\tset       mode <auto|comfort|eco>\n");
}

int read_config()
{
    FILE *fp = fopen(MAX_CONFIG_FILE, "r");
    int res = parse_file(fp);

    fclose(fp);

    return res;
}

int get(const char* program, struct sockaddr_in* serv_addr,
        int argc, char *argv[])
{
    return 0;
}

int set(const char* program, struct sockaddr_in* serv_addr,
        int argc, char *argv[])
{
    int sockfd = 0, n = 0;
    MAX_msg_list* msg_list = NULL;
    struct MAX_message *m_s, *m_l;
    struct l_Data *l_d;
    int mode;
    int connectionId;

    if (argc != 3)
    {
        help(program);
        return 1;
    }

    if (strcmp(argv[1], "mode") != 0)
    {
        printf("Error : Invalid parameter for command\n");
        help(program);
        return 1;
    }

    if (strcmp(argv[1], "auto") == 0)
    {
        mode = Auto;
    }
    else if (strcmp(argv[1], "comfort") == 0)
    {
        mode = Comfort;
    }
    else if (strcmp(argv[1], "eco") == 0)
    {
        mode = Eco;
    }

    /* Read config file */
    if (read_config() != 0)
    {
        printf("Error : Invalid configuration file\n");
        return 1;
    }
return 0;

    /* Connect to cube */
    if ((connectionId = MAXConnect((struct sockaddr*)serv_addr)) < 0)
    {
        printf("Error : Could not connect to MAX!cube\n");
        return 1;
    } 

    /* Wait for Hello message */
    if (MaxMsgRecv(connectionId, &msg_list, MSG_TMO) < 0)
    {
        printf("Error : Hello message not received from MAX!cube\n");
        return 1;
    }

#ifdef MAX_DEBUG
    dumpMAXpkt(msg_list);
#endif

    if (MAXDisconnect(connectionId) < 0)
    {
        printf("Error : Failed to close connection with MAX!cube\n");
        return 1;
    }

return 0;

/*
set:
*/
    m_s = create_s_cmd(&n);
    write(sockfd, m_s, n);
    free(m_s);
    m_l = malloc(sizeof(struct MAX_message) - 1 + sizeof(struct l_Data));
    m_l->type = 'l';
    m_l->colon = ':';
    l_d = (struct l_Data*)m_l->data;
    memcpy(l_d, MSG_END, sizeof(MSG_END));
    write(sockfd, m_l, sizeof(struct MAX_message) - 1 + sizeof(struct l_Data));
    free(m_l);
    return 0;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in serv_addr;

    if(argc < 4)
    {
        help(argv[0]);
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2])); 

    printf("Welcome MAX! cube\n");

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("Error : invalid address\n");
        help(argv[0]);
        return 1;
    } 

    if (strcmp(argv[3], "get") == 0)
    {
       return get(argv[0], &serv_addr, argc - 3, &argv[3]);
    }
    else if (strcmp(argv[3], "set") == 0)
    {
       return set(argv[0], &serv_addr, argc - 3, &argv[3]);
    }

    help(argv[0]);

    return 0;
}

