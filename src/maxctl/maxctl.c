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

#define MSG_TMO 500      /* Message receive timeout */

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

struct mode_param {
    int mode;
    char *device_id;
};

void help(const char* program)
{
    printf("Usage: %s <ip of MAX! cube> <port pf MSX! cube> <command> " \
           "<params>\n", program);
    printf("\tCommands  Params\n" \
           "\tget       status\n" \
           "\tset       mode <auto|comfort|eco> all|<device_id>\n" \
           "\tset       program all|<device_id>\n");
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

int eval_S_response(MAX_msg_list* msg_list)
{
    struct S_Data *S_D;
    while (msg_list != NULL) {
        S_D = (struct S_Data*)msg_list->MAX_msg->data;
        if (S_D->Command_Result[0] != '0')
        {
            return -1;
        }
        msg_list = msg_list->next;
    }

    return 0;
}

/* param -  pointer to struct s_Program_Data */
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
    struct s_Program_Data *s_Program_Data =
        (struct s_Program_Data*)((struct send_param*)param)->data;

    if (cl == NULL)
    {
        return -1;
    }

    as = &cl->auto_schedule;
    if (as->skip != 0)
    {
#ifdef MAX_DEBUG
        printf("    unchanged schedule, send nothing\n");
#endif
        return 0;
    }
    if (as->day < 0 || as->day >= sizeof(week_days) / sizeof(week_days[0]))
    {
        return -1;
    }

    s_Program_Data->Day_of_week[0] = as->day;
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
        s_Program_Data->Temp_and_Time[i] = ((temp << 1) | ((t >> 8) & (0x1)));
        s_Program_Data->Temp_and_Time[i + 1] = (t & 0xff);
        i += 2;
        program = program->next;
        if (i >= (MAX_CMD_SETPOINTS * 2))
        {
            break;
        }
    }

    /* Create packet */
    off = sizeof(struct MAX_message) - 1;
    m_s = (struct MAX_message*)hex_to_base64(
           (const unsigned char*)s_Program_Data,
           sizeof(*s_Program_Data), off, MSG_END_LEN, &outlen);
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
    if (res != 0)
    {
        return res;
    }

    /* Wait for S response */
    if (MaxMsgRecv(connectionId, &msg_list) < 0)
    {
        return 0;
    }
#ifdef MAX_DEBUG
    dumpMAXHostpkt(msg_list);
#endif
    res = eval_S_response(msg_list);
    freeMAXpkt(&msg_list);
    if (res != 0)
    {
        printf("Error : 'S' command discarded\n");
        /* Don't return here, call close session gracefully */
    }
    
    return res;
}

int send_mode(union cfglist *cl, void *param)
{
    struct s_Temp_Mode_Data s_Temp_Mode_Data;
    struct send_param *send_param = (struct send_param*)param;
    struct mode_param *mode_param = (struct mode_param*)send_param->data;
    uint32_t rf_address;
    struct config *config;
    int off, res;
    size_t outlen;
    struct MAX_message *m_s;
    MAX_msg_list *msg_list = NULL;
    int connectionId = ((struct send_param*)param)->connectionId;

    /* Send Temp and Mode */
    /* Initialize base string */
    memset(&s_Temp_Mode_Data, 0, sizeof(s_Temp_Mode_Data));
    memcpy(s_Temp_Mode_Data.Base_String,
           base_string_code(TemperatureAndMode), BS_CODE_SZ);

    if (cl == NULL || cl->ruleset.device_config == NULL)
    {
        return -1;
    }
    config = &cl->ruleset.device_config->config;

    rf_address = cl->ruleset.device_config->rf_address;
    if (strcmp(mode_param->device_id, "all") != 0)
    {
        /* mode not applied to all devices */
        uint32_t device_id = strtol(mode_param->device_id, NULL, 16);
        if (device_id != rf_address)
        {
            /* skip this device */
            return 0;
        }
    }
    printf("device: %x, send_mode mode: %d, device_id: %s\n", rf_address,
        mode_param->mode, mode_param->device_id);

    s_Temp_Mode_Data.RF_Address[0] =
        (cl->ruleset.device_config->rf_address >> 16) & 0xff;
    s_Temp_Mode_Data.RF_Address[1] =
        (cl->ruleset.device_config->rf_address >> 8) & 0xff;
    s_Temp_Mode_Data.RF_Address[2] =
        cl->ruleset.device_config->rf_address & 0xff;

    s_Temp_Mode_Data.Room_Nr[0] = config->room_id;
    
    switch (mode_param->mode)
    {
        case AutoMode:
            s_Temp_Mode_Data.Temp_and_Mode[0] =
                (0b11000000 & (AutoTempMode << 6)) | 
                (0b00111111 & 0);
            break;
        case EcoMode:
            s_Temp_Mode_Data.Temp_and_Mode[0] =
                (0b11000000 & (ManualTempMode << 6)) | 
                (0b00111111 & (int)(config->eco_temp * 2));
            break;
        case ComfortMode:
            s_Temp_Mode_Data.Temp_and_Mode[0] =
                (0b11000000 & (ManualTempMode << 6)) | 
                (0b00111111 & (int)(config->comfort_temp * 2));
            break;
        default:
            return 1;
    }

    /* Create packet */
    off = sizeof(struct MAX_message) - 1;
    m_s = (struct MAX_message*)hex_to_base64(
           (const unsigned char*)&s_Temp_Mode_Data,
           sizeof(s_Temp_Mode_Data), off, MSG_END_LEN, &outlen);
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

    /* Wait for S response */
    if (MaxMsgRecv(connectionId, &msg_list) < 0)
    {
        return 0;
    }
#ifdef MAX_DEBUG
    dumpMAXHostpkt(msg_list);
#endif
    res = eval_S_response(msg_list);
    freeMAXpkt(&msg_list);
    if (res != 0)
    {
        printf("Error : 'S' command discarded\n");
        /* Don't return here, call close session gracefully */
    }
    return res;
}

int send_ruleset(union cfglist *cl, void *param)
{
    uint32_t rf_address;
    struct config *config;
    struct auto_schedule *as;
    struct s_Program_Data s_Program_Data;
    struct s_Eco_Temp_Data s_Eco_Temp_Data;
    struct send_param *send_param = param, sched_param;
    int off, res;
    size_t outlen;
    struct MAX_message *m_s;
    MAX_msg_list *msg_list = NULL;
    int connectionId = send_param->connectionId;

    /* Send Program / weekly schedule */
    /* Initialize base string */
    memset(&s_Program_Data, 0, sizeof(s_Program_Data));
    memcpy(s_Program_Data.Base_String,
           base_string_code(ProgramData), BS_CODE_SZ);

    if (cl == NULL || cl->ruleset.device_config == NULL)
    {
        return -1;
    }
    config = &cl->ruleset.device_config->config;

    rf_address = cl->ruleset.device_config->rf_address;
    if (strcmp((const char*)(send_param->data), "all") != 0)
    {
        /* mode not applied to all devices */
        uint32_t device_id = strtol((const char*)(send_param->data), NULL, 16);
        if (device_id != rf_address)
        {
            /* skip this device */
            return 0;
        }
    }
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
    s_Program_Data.RF_Address[0] =
        (cl->ruleset.device_config->rf_address >> 16) & 0xff;
    s_Program_Data.RF_Address[1] =
        (cl->ruleset.device_config->rf_address >> 8) & 0xff;
    s_Program_Data.RF_Address[2] =
        cl->ruleset.device_config->rf_address & 0xff;

    s_Program_Data.Room_Nr[0] = config->room_id;

    as = config->auto_schedule;
    if (as == NULL)
    {
#ifdef MAX_DEBUG
        printf("    empty schedule, send nothing\n");
#endif
    }
    
    /* Join data to send params */
    sched_param.connectionId = send_param->connectionId;
    sched_param.data = (void*)&s_Program_Data;
    walklist((union cfglist*)as, send_auto_schedule, &sched_param);

    if (config->skip != 0)
    {
#ifdef MAX_DEBUG
        printf("    unchanged config, send nothing\n");
#endif
        goto skip_config;
    }
    /* Send Eco Temp */
    /* Temperature comfort, temperature eco, temperature max, temperature min,
     * temperature window open */
    /* Initialize base string */
    memset(&s_Eco_Temp_Data, 0, sizeof(s_Eco_Temp_Data));
    memcpy(s_Eco_Temp_Data.Base_String,
           base_string_code(EcoModeTemperature), BS_CODE_SZ);

    s_Eco_Temp_Data.RF_Address[0] =
        (cl->ruleset.device_config->rf_address >> 16) & 0xff;
    s_Eco_Temp_Data.RF_Address[1] =
        (cl->ruleset.device_config->rf_address >> 8) & 0xff;
    s_Eco_Temp_Data.RF_Address[2] =
        cl->ruleset.device_config->rf_address & 0xff;

    s_Eco_Temp_Data.Room_Nr[0] = config->room_id;

#define TEMP_MAX 30.5
#define TEMP_MIN 4.5
#define TEMP_OFF 0
#define TEMP_WINDOW_OPEN 12
#define DUR_WINDOW_OPEN 15
    s_Eco_Temp_Data.Temperature_Comfort[0] = (unsigned char)(config->comfort_temp * 2);
    s_Eco_Temp_Data.Temperature_Eco[0] = (unsigned char)(config->eco_temp * 2);
    s_Eco_Temp_Data.Temperature_Max[0] = (unsigned char)(TEMP_MAX * 2);
    s_Eco_Temp_Data.Temperature_Min[0] = (unsigned char)(TEMP_MIN * 2);
    s_Eco_Temp_Data.Temperature_Offset[0] = (unsigned char)((TEMP_OFF + 3.5) * 2);
    s_Eco_Temp_Data.Temperature_Window_Open[0] = (unsigned char)(TEMP_WINDOW_OPEN * 2);
    s_Eco_Temp_Data.Duration_Window_Open[0] = (unsigned char)(DUR_WINDOW_OPEN / 5);

    /* Create packet */
    off = sizeof(struct MAX_message) - 1;
    m_s = (struct MAX_message*)hex_to_base64(
           (const unsigned char*)&s_Eco_Temp_Data,
           sizeof(s_Eco_Temp_Data), off, MSG_END_LEN, &outlen);
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

    /* Wait for S response */
    if (MaxMsgRecv(connectionId, &msg_list) < 0)
    {
        return 0;
    }
#ifdef MAX_DEBUG
    dumpMAXHostpkt(msg_list);
#endif
    res = eval_S_response(msg_list);
    freeMAXpkt(&msg_list);
    if (res != 0)
    {
        printf("Error : 'S' command discarded\n");
        /* Don't return here, call close session gracefully */
    }
skip_config:
    return res;
}

int read_config(struct ruleset **ruleset)
{
    FILE *fp = fopen(MAX_CONFIG_FILE, "r");
    int res = parse_file(fp, ruleset);

    fclose(fp);

    return res;
}

int get_status(const char* program, struct sockaddr_in* serv_addr,
        int argc, char *argv[])
{
    MAX_msg_list* msg_list = NULL;
    int connectionId;
 
    if (argc != 1)
    {
        help(program);
        return 1;
    }

    /* Open connection and send configuration */
    /* Connect to cube */
    if ((connectionId = MAXConnect((struct sockaddr*)serv_addr)) < 0)
    {
        printf("Error : Could not connect to MAX!cube\n");
        return 1;
    }

    /* Wait for Hello message */
    if (MaxMsgRecvTmo(connectionId, &msg_list, MSG_TMO) < 0)
    {
        printf("Error : Hello message not received from MAX!cube\n");
        return 1;
    }

    dumpMAXHostpkt(msg_list);
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

    return 0;
}

int get(const char* program, struct sockaddr_in* serv_addr,
        int argc, char *argv[])
{
    if (argc < 2)
    {
        help(program);
        return 1;
    }

    if (strcmp(argv[1], "status") == 0)
    {
        return get_status(program, serv_addr, argc - 1, &argv[1]);
    }

    printf("Error : Invalid parameter for command\n");
    help(program);
    return 1;
}

int set_program(const char* program, struct sockaddr_in* serv_addr,
        int argc, char *argv[])
{
    struct ruleset *rs;
    int connectionId;
    MAX_msg_list* msg_list = NULL;
    struct send_param send_param;
    int result = 0;

    if (argc != 2)
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
    if (MaxMsgRecvTmo(connectionId, &msg_list, MSG_TMO) < 0)
    {
        printf("Error : Hello message not received from MAX!cube\n");
        return 1;
    }

#ifdef MAX_DEBUG
    dumpMAXHostpkt(msg_list);
#endif
    /* Flag rules that updates configuration. We don't send unchanged
     * parameters */
    walklist((union cfglist*)rs, flag_ruleset, msg_list);
    freeMAXpkt(&msg_list);

    /* Pack some params needed to send function */
    send_param.connectionId = connectionId;
    send_param.data = argv[1];
    /* Send program configuration */
    walklist((union cfglist*)rs, send_ruleset, &send_param);

    /* Send 'q' (quit) command*/
    msg_list = create_quit_pkt(connectionId);
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

    return result;
}

int set_mode(const char* program, struct sockaddr_in* serv_addr,
        int argc, char *argv[])
{
    MAX_msg_list* msg_list = NULL;
    int mode;
    int connectionId;
    struct ruleset *rs;
    struct send_param send_param;
    struct mode_param mode_param;
    int result = 0;

    if (argc != 3)
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
    else
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
    if (MaxMsgRecvTmo(connectionId, &msg_list, MSG_TMO) < 0)
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
    send_param.data = &mode_param;
    mode_param.mode = mode;
    mode_param.device_id = argv[2];
    /* Send program configuration */
    walklist((union cfglist*)rs, send_mode, &send_param);

    /* Send 'q' (quit) command*/
    msg_list = create_quit_pkt(connectionId);
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

    return result;
}

int set(const char* program, struct sockaddr_in* serv_addr,
        int argc, char *argv[])
{
    if (argc < 2)
    {
        help(program);
        return 1;
    }

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

