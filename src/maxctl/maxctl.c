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
#define MSG_END_LEN 2  /* Message terminator sequence len */
#define MSG_TMO 2      /* Message receive timeout */

enum Mode
{
    AutoMode = 0,
    ComfortMode = 1,
    EcoMode = 2
};

static char* week_days[] = {
    "Saturday",
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday"
};

struct send_param {
    int connectionId;
    void *data;
};

void help(const char* program)
{
    printf("Usage: %s <ip of MAX! cube> <port pf MSX! cube> <command> <params>\n", program);
    printf("\tCommands  Params\n" \
           "\tget       status\n" \
           "\tset       mode <auto|comfort|eco>\n" \
           "\tset       program\n");
}

MAX_msg_list* create_quit_pkt(int connectionId)
{
    struct MAX_message *m_q;
    struct q_Data *q_d;
    size_t len;
    
    len = sizeof(struct MAX_message) - 1 + sizeof(struct q_Data);
    m_q = malloc(len);
    m_q->type = 'q';
    m_q->colon = ':';
    q_d = (struct q_Data*)m_q->data;
    memcpy(q_d, MSG_END, MSG_END_LEN);
    return appendMAXmsg(NULL, m_q, len);
}

/* param -  pointer to struct s_Data */
int send_auto_schedule(union cfglist *cl, void *param)
{
    struct auto_schedule *as;
    struct program *program;
    int temp, hour, minutes, t;
    int i, off, res;
    size_t outlen;
    struct MAX_message *m_s;
    MAX_msg_list *msg_list = NULL;
    int connectionId = ((struct send_param*)param)->connectionId;
    struct s_Data *s_Data = (struct s_Data*)((struct send_param*)param)->data;

    if (cl == NULL)
    {
        return -1;
    }

    as = &cl->auto_schedule;
    if (as->day < 0 || as->day >= sizeof(week_days) / sizeof(week_days[0]))
    {
        return -1;
    }

    s_Data->Day_of_week[0] = as->day;
#ifdef MAX_DEBUG
    printf("    packing schedule\n");
#endif
    program = as->schedule;
    if (program == NULL)
    {
#ifdef MAX_DEBUG
        printf("    empty schedule, send nothing\n");
#endif
        return 0;
    }
    /* Pack the daily program here */
    i = 0;
    while (program != NULL)
    {
        temp = (int)(program->temperature * 2);
        hour = program->hour;
        minutes = program->minutes;
        t = (60 * hour + minutes) / 5;
        /* Use first fiels Temp_and_Time to write into next as well */
        s_Data->Temp_and_Time[i] = ((temp << 1) | ((t >> 8) & (0x1)));
        s_Data->Temp_and_Time[i + 1] = (t & 0xff);
        i += 2;
        program = program->next;
        if (i >= (MAX_CMD_SETPOINTS * 2))
        {
            break;
        }
    }

    /* Send here */
    off = sizeof(struct MAX_message) - 1;
    m_s = (struct MAX_message*)hex_to_base64((const unsigned char*)s_Data,
           sizeof(*s_Data), off, MSG_END_LEN, &outlen);
    m_s->type = 's';
    m_s->colon = ':';
    memcpy(&m_s->data[outlen], MSG_END, MSG_END_LEN);
    msg_list = appendMAXmsg(NULL, m_s, off + outlen + MSG_END_LEN);
#ifdef MAX_DEBUG
    dumpMAXNetpkt(msg_list);
#endif
    /* Send message */
    res = MAXMsgSend(connectionId, msg_list);
    freeMAXpkt(&msg_list);
    
    return res;
}

int send_ruleset(union cfglist *cl, void *param)
{
    struct config *config;
    struct auto_schedule *as;
    struct s_Data s_Data;
    struct send_param *send_param = param;

    /* Initialize base string */
    memset(&s_Data, 0, sizeof(s_Data));
    memcpy(s_Data.Base_String,  base_string_code(ProgramData), BS_CODE_SZ);

    if (cl == NULL || cl->ruleset.device_config == NULL)
    {
        return -1;
    }
    config = &cl->ruleset.device_config->config;
#ifdef MAX_DEBUG
    printf("sending device %x\n", cl->ruleset.device_config->rf_address);
#endif
    if (config->room_id == NOT_CONFIGURED_UL)
    {
#ifdef MAX_DEBUG
        printf("empty configuration, send nothing\n");
#endif
        return 0;
    }
    s_Data.RF_Address[0] =
        (cl->ruleset.device_config->rf_address >> 16) & 0xff;
    s_Data.RF_Address[1] =
        (cl->ruleset.device_config->rf_address >> 8) & 0xff;
    s_Data.RF_Address[2] =
        cl->ruleset.device_config->rf_address & 0xff;

    s_Data.Room_Nr[0] = cl->ruleset.device_config->config.room_id;

    as = config->auto_schedule;
    if (as == NULL)
    {
#ifdef MAX_DEBUG
        printf("    empty schedule, send nothing\n");
#endif
    }

    /* Join data to send params */
    send_param->data = (void*)&s_Data;
    walklist((union cfglist*)as, send_auto_schedule, param);

    return 0;
}

int read_config(struct ruleset **ruleset)
{
    FILE *fp = fopen(MAX_CONFIG_FILE, "r");
    int res = parse_file(fp, ruleset);

    fclose(fp);

    return res;
}

int get(const char* program, struct sockaddr_in* serv_addr,
        int argc, char *argv[])
{
    return 0;
}

int set_program(const char* program, struct sockaddr_in* serv_addr,
        int argc, char *argv[])
{
    struct ruleset *rs;
    int connectionId;
    MAX_msg_list* msg_list = NULL;
    struct send_param send_param;

    if (argc != 1)
    {
        help(program);
        return 1;
    }

    /* Read config file */
    if (read_config(&rs) != 0)
    {
        printf("Error : cannot read configuration\n");
        return 1;
    }

#ifdef MAX_DEBUG
    /* Dump rules to check configuration */
    walklist((union cfglist*)rs, dump_ruleset, NULL);
#endif

    /* Open connection and send configuration */
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
    dumpMAXHostpkt(msg_list);
#endif
    freeMAXpkt(&msg_list);

    /* Pack some params needed to send function */
    send_param.connectionId = connectionId;
    /* Send program configuration */
    walklist((union cfglist*)rs, send_ruleset, &send_param);

    /* Wait for S response */
    if (MaxMsgRecv(connectionId, &msg_list, MSG_TMO) < 0)
    {
        printf("Error : Hello message not received from MAX!cube\n");
        return 1;
    }
#ifdef MAX_DEBUG
    dumpMAXHostpkt(msg_list);
#endif
    freeMAXpkt(&msg_list);

    /* Send 'q' (quit) command*/
    msg_list = create_quit_pkt(connectionId);
    /* Wait for Hello message */
    if (MAXMsgSend(connectionId, msg_list) < 0)
    {
        printf("Error : Hello message not received from MAX!cube\n");
        /* Don't return here, call MAXDisconnect */
    }
    freeMAXpkt(&msg_list);

    if (MAXDisconnect(connectionId) < 0)
    {
        printf("Error : Failed to close connection with MAX!cube\n");
        return 1;
    }

    /* Free configuration data */
    walklist((union cfglist*)rs, free_ruleset, NULL);

    return 0;
}

int set_mode(const char* program, struct sockaddr_in* serv_addr,
        int argc, char *argv[])
{
    MAX_msg_list* msg_list = NULL;
    int mode;
    int connectionId;

    if (argc != 2)
    {
        help(program);
        return 1;
    }

    if (strcmp(argv[1], "auto") == 0)
    {
        mode = AutoMode;
    }
    else if (strcmp(argv[1], "comfort") == 0)
    {
        mode = ComfortMode;
    }
    else if (strcmp(argv[1], "eco") == 0)
    {
        mode = EcoMode;
    }

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
    dumpMAXHostpkt(msg_list);
#endif

    if (MAXDisconnect(connectionId) < 0)
    {
        printf("Error : Failed to close connection with MAX!cube\n");
        return 1;
    }

    return 0;
}

int set(const char* program, struct sockaddr_in* serv_addr,
        int argc, char *argv[])
{
    if (strcmp(argv[1], "mode") == 0)
    {
        return set_mode(program, serv_addr, argc - 1, &argv[1]);
    }

    if (strcmp(argv[1], "program") == 0)
    {
        return set_program(program, serv_addr, argc - 1, &argv[1]);
    }

    printf("Error : Invalid parameter for command\n");
    help(program);
    return 1;
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

