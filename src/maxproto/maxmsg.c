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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "maxmsg.h"

/* Maximum number of set points per day */
#define MAX_DAY_SETPOINTS 13

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
    "Friday"
};

typedef struct BS{
    char value[6];
} BaseString;

static BaseString bs_code[] = {
                             /* temperature and mode setting */
                             {{0x00, 0x04, 0x40, 0x00, 0x00, 0x00}},
                             /* program data setting */
                             {{0x00, 0x04, 0x10, 0x00, 0x00, 0x00}},
                             /* eco mode temperature setting */
                             {{0x00, 0x00, 0x11, 0x00, 0x00, 0x00}},
                             /* config valve functions */
                             {{0x00, 0x04, 0x12, 0x00, 0x00, 0x00}},
                             /* add link partner */
                             {{0x00, 0x00, 0x20, 0x00, 0x00, 0x00}},
                             /* remove link partner */
                             {{0x00, 0x00, 0x21, 0x00, 0x00, 0x00}},
                             /* set group address */
                             {{0x00, 0x00, 0x22, 0x00, 0x00, 0x00}}
                         };

static char* bs_name[] = {
                             "temperature and mode",
                             "program data",
                             "eco mode temperature",
                             "config valve functions",
                             "add link partner",
                             "remove link partner",
                             "set group address"
                         };

/* Declare this as extern to avoid make it public in the headers */
extern int parseMAXData(char *MAXData, int size, MAX_msg_list** msg_list);

/* Dump packet in host format */
void dumpMAXHostpkt(MAX_msg_list* msg_list)
{
    char buf[1024];
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
                    printf("<<<<<<<<<<<<<<<<<<<< RX <<<<<<<<<<<<<<<<<<<<\n");
                    snprintf(buf, sizeof(H_D->Serial_number) + 1,
                             H_D->Serial_number);
                    printf("\tSerial no           %s\n", buf);
                    snprintf(buf, sizeof(H_D->RF_address) + 1,
                             H_D->RF_address);
                    printf("\tRF address          %s\n", buf);
                    snprintf(buf, sizeof(H_D->Firmware_version) + 1,
                             H_D->Firmware_version);
                    printf("\tFirmware version    %s\n", buf);
                    snprintf(buf, sizeof(H_D->unknown) + 1, H_D->unknown);
                    printf("\tunknown             %s\n", buf);
                    snprintf(buf, sizeof(H_D->HTTP_connection_id),
                             H_D->HTTP_connection_id);
                    printf("\tHTTP connection id  %s\n", buf);
                    snprintf(buf, sizeof(H_D->Duty_cycle) + 1,
                             H_D->Duty_cycle);
                    printf("\tDuty cycle          %s\n", buf);
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
                    snprintf(buf, sizeof(H_D->State_Cube_Time) + 1,
                             H_D->State_Cube_Time);
                    printf("\tState Cube Time     %s\n", buf);
                    snprintf(buf, sizeof(H_D->NTP_Counter) + 1,
                             H_D->NTP_Counter);
                    printf("\tNTP Counter         %s\n", buf);
                    printf("<<<<<<<<<<<<<<<<<<<< RX <<<<<<<<<<<<<<<<<<<<\n");
                    break;
                }
            case 'C':
                {
                    struct C_Data *C_D = (struct C_Data*)md;
                    unsigned char *tmp;
                    char *str;
                    int val;
                    union C_Data_Device *data =
                        (union C_Data_Device*)(md + sizeof(struct C_Data));

                    printf("<<<<<<<<<<<<<<<<<<<< RX <<<<<<<<<<<<<<<<<<<<\n");
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

                            snprintf(buf, sizeof(C_D->RF_address) + 1,
                                     C_D->RF_address);
                            printf("\tRF address          %s\n", buf);

                            val = data->device.Data_Length[0];
                            printf("\tData Length         %d\n", val);

                            tmp = data->device.Address_of_device;
                            printf("\tAddress_of_device   %x%x%x\n",
                                   tmp[0], tmp[1], tmp[2]);

                            val = data->device.Device_Type[0];
                            printf("\tDevice Type         %s\n",
                                   device_types[val]);

                            val = data->device.Room_ID[0];
                            printf("\tRoom ID             %d\n", val);

                            val = data->device.Firmware_version[0];
                            printf("\tFirmware version    %d\n", val);

                            val = data->device.Test_Result[0];
                            printf("\tTest Result         %d\n", val);

                            snprintf(buf,
                                     sizeof(data->device.Serial_Number) + 1,
                                     data->device.Serial_Number);
                            printf("\tSerial Number       %s\n", buf);

                            fval = config->rtc.Comfort_Temperature[0] / 2.;
                            printf("\tComfort Temperature %.1f\n", fval);
                            fval = config->rtc.Eco_Temperature[0] / 2.;
                            printf("\tEco Temperature     %.1f\n", fval);
                            fval = config->rtc.Temperature_offset[0] / 2. - 3.5;
                            printf("\tTemperature offset  %.1f\n", fval);
                            day = config->rtc.Decalcification[0] >> 5;
                            hours = (config->rtc.Decalcification[0] & 0b11111);
                            printf("\tDecalcification     %s %2d:00\n",
                                   week_days[day], hours);
                            memcpy(&ws, &config->rtc.Weekly_Program[0],
                                   sizeof(ws));
                            s = 0;
                            day = 0;
                            printf("\tWeekly Program\n");
                            while (s < sizeof(config->rtc.Weekly_Program))
                            {
                                fval = (config->rtc.Weekly_Program[s] >> 1) /
                                       2.;
                                ws = (((config->rtc.Weekly_Program[s] & 1) << 8)
                                     | config->rtc.Weekly_Program[s + 1]) * 5;
                                hours = ws / 60;
                                mins = ws %60;
                                printf("\t\t%-10s  %.1f until %02d:%02d\n",
                                       week_days[day], fval, hours, mins);
                                if (hours >= 24)
                                {
                                    s = (s / (MAX_DAY_SETPOINTS * 2) + 1) *
                                        MAX_DAY_SETPOINTS* 2;
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

                            snprintf(buf, sizeof(C_D->RF_address) + 1,
                                     C_D->RF_address);
                            printf("\tRF address          %s\n", buf);

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

                            snprintf(buf,
                                     sizeof(data->cube.Serial_Number) + 1,
                                     data->cube.Serial_Number);
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
                    printf("<<<<<<<<<<<<<<<<<<<< RX <<<<<<<<<<<<<<<<<<<<\n");
                    break;
                }
            case 's':
                {
                    struct s_Data *s_D = (struct s_Data*)md;
                    unsigned char *tmp;
                    float fval;
                    uint32_t val, hours, mins;
                    uint16_t ws, idx, s;

                    printf(">>>>>>>>>>>>>>>>>>>> TX >>>>>>>>>>>>>>>>>>>>\n");
                    snprintf(buf, sizeof(s_D->Base_String) + 1,
                            s_D->Base_String);
                    idx = base_string_index(buf);
                    printf("\tSet                 %s\n", bs_name[idx]);

                    tmp = s_D->RF_Address;
                    printf("\tRF address          %x%x%x\n",
                           tmp[0], tmp[1], tmp[2]);

                    val = s_D->Room_Nr[0];
                    printf("\tRoom Nr             %d\n", val);

                    val = s_D->Day_of_week[0];
                    printf("\tDay Program         %s\n", week_days[val]);

                    s = 0;
                    while (s < MAX_CMD_SETPOINTS * 2)
                    {
                        fval = (s_D->Temp_and_Time[s] >> 1) / 2.;
                        ws = (((s_D->Temp_and_Time[s] & 1) << 8) |
                                s_D->Temp_and_Time[s + 1]) * 5;
                        hours = ws / 60;
                        mins = ws %60;
                        printf("\t\t\t    %.1f until %02d:%02d\n",
                               fval, hours, mins);
                        s += 2;
                        if (hours >= 24)
                        {
                            break;
                        }
                    }
                    printf(">>>>>>>>>>>>>>>>>>>> TX >>>>>>>>>>>>>>>>>>>>\n");
                    break;
                }
            case 'l':
                {
                    printf(">>>>>>>>>>>>>>>>>>>> TX >>>>>>>>>>>>>>>>>>>>\n");
                    printf(">>>>>>>>>>>>>>>>>>>> TX >>>>>>>>>>>>>>>>>>>>\n");
                    break;
                }
            default:
                break;
        }
        msg_list = msg_list->next;
    }
}

/* Dump packet in network format */
void dumpMAXNetpkt(MAX_msg_list* msg_list)
{
    char buff[4096];
    MAX_msg_list *tmpmsg_list = NULL;

    while (msg_list != NULL) {
        strcpy(buff, (char*)msg_list->MAX_msg);
        parseMAXData(buff, strlen(buff), &tmpmsg_list);
        msg_list = msg_list->next;
    }
    dumpMAXHostpkt(tmpmsg_list);
    /* free the temporary allocated data */
    free(tmpmsg_list);
}

void freeMAXpkt(MAX_msg_list** msg_list)
{
    MAX_msg_list *msg = NULL, *iter = *msg_list;

    while (iter != NULL) {
        msg = iter;
        iter = iter->next;
        free(msg->MAX_msg);
        free(msg);
    }
    *msg_list = NULL;
}

MAX_msg_list* appendMAXmsg(MAX_msg_list* msg_list, struct MAX_message *msg,
    size_t msg_len)
{
    MAX_msg_list *newmsg = (MAX_msg_list*)malloc(sizeof(MAX_msg_list));
    MAX_msg_list *resmsg = newmsg;

    if (msg_list != NULL)
    {
        resmsg = msg_list;
        while (msg_list->next != NULL) {
            msg_list = msg_list->next;
        }
        msg_list->next = newmsg;
    }
    newmsg->MAX_msg = msg;
    newmsg->MAX_msg_len = msg_len;
    newmsg->prev = msg_list;
    newmsg->next = NULL;
    return resmsg;
}

int base_string_index(const char *base_string)
{
    int i;

    for(i = 0; i < sizeof(bs_code) / sizeof(bs_code[0]); i++)
    {
        if (strncmp(bs_code[i].value, base_string, BS_CODE_SZ) == 0)
        {
            return i;
        }
    }

    return -1;
}

const char*  base_string_code(int index)
{
    if (index < 0 || index > sizeof(bs_code) / sizeof(bs_code[0]))
    {
       return NULL;
    }
    return bs_code[index].value;
}
