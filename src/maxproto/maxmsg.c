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

static char* temp_mode_str[] = {
    "auto/weekly program",
    "manual",
    "vacation",
    "boost"
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
                    printf("\tDuty cycle          %d%%\n",
                           (int) strtol(buf, NULL, 16));
                    snprintf(buf, sizeof(H_D->Free_Memory_Slots) + 1,
                             H_D->Free_Memory_Slots);
                    printf("\tFree Memory Slots   %d\n",
                            (int) strtol(buf, NULL, 16));
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
                            printf("\tAddress of device   %x%x%x\n",
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
                            fval = config->rtc.Max_Set_Point_Temperature[0] / 2.;
                            printf("\tMax Set Point Temp  %.1f\n", fval);
                            fval = config->rtc.Min_Set_Point_Temperature[0] / 2.;
                            printf("\tMin Set Point Temp  %.1f\n", fval);
                            fval = config->rtc.Temperature_offset[0] / 2. - 3.5;
                            printf("\tTemperature offset  %.1f\n", fval);
                            fval = config->rtc.Window_Open_Temperature[0] / 2.;
                            printf("\tWindow Open Temp    %.1f\n", fval);
                            val = config->rtc.Window_Open_Duration[0] * 5;
                            printf("\tWindow Open Dur     %d min\n", val);
                            day = config->rtc.Decalcification[0] >> 5;
                            hours = (config->rtc.Decalcification[0] & 0b11111);
                            printf("\tDecalcification     %s %2d:00\n",
                                   week_days[day], hours);
                            fval = config->rtc.Max_Valve_Setting[0] *
                                100 / 255.;
                            printf("\tMax Valve Setting   %.1f\n", fval);
                            fval = config->rtc.Valve_Offset[0] * 100 / 255.;
                            printf("\tValve Offset        %.1f\n", fval);
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
            case 'L':
                {
                    struct L_Data *L_D;
                    unsigned char *tmp;
                    int val, len, pos, tlen;
                    float fval;
                    size_t hdr_sz = sizeof(struct MAX_message) - 1;
                    
                    printf("<<<<<<<<<<<<<<<<<<<< RX <<<<<<<<<<<<<<<<<<<<\n");
                    pos = 0;
                    tlen = msg_list->MAX_msg_len - hdr_sz;
                    printf("\tTotal length        %d\n", tlen);
                    while (1)
                    {
                        char l_end[] = {0xce, 0x00};
                        int mode;
                        if (memcmp(md + pos, l_end, sizeof(l_end)) == 0)
                        {
                            /* Found terminator, exit */
                            break;
                        }
                        L_D = (struct L_Data*)(md + pos);
                        val = L_D->Submessage_Length[0];
                        len = val;
                        printf("\tSubmessage Length   %d\n", val);
                        tmp = L_D->RF_Address;
                        printf("\tRF address          %x%x%x\n",
                               tmp[0], tmp[1], tmp[2]);
                        val = L_D->Flags[0];
                        val = (val << 8) + L_D->Flags[1];
                        mode = val & 0b00000011;
                        printf("\tTemp mode           %s\n",
                               temp_mode_str[mode]);
                        if (len > 6)
                        {
                            /* More info available */
                            val = L_D->Valve_Position[0];
                            printf("\tValve Position      %d%%\n", val);
                            fval = L_D->Temperature[0] / 2.;
                            printf("\tTemperature         %.1f\n", fval);
                            if (mode == AutoTempMode)
                            {
                                val = L_D->next_data[0] & 0b00000001;
                                val = (val << 8) + L_D->next_data[1];
                                fval = val / 10.;
                                printf("\tActual temperature  %.1f\n", fval);
                            }
                        }
                        pos += len + 1;
                        if (pos >= tlen)
                        {
                            break;
                        }
                        printf("\t----------------------\n");
                    }
                    printf("<<<<<<<<<<<<<<<<<<<< RX <<<<<<<<<<<<<<<<<<<<\n");
                    break;
                }
            case 'S':
                {
                    struct S_Data *S_D = (struct S_Data*)md;
                    printf("<<<<<<<<<<<<<<<<<<<< RX <<<<<<<<<<<<<<<<<<<<\n");
                    snprintf(buf, sizeof(S_D->Duty_Cycle) + 1,
                             S_D->Duty_Cycle);
                    printf("\tDuty Cycle          %d%%\n",
                           (int) strtol(buf, NULL, 16));
                    snprintf(buf, sizeof(S_D->Command_Result) + 1,
                             S_D->Command_Result);
                    printf("\tCommand Result      %s\n", buf);
                    snprintf(buf, sizeof(S_D->Free_Memory_Slots) + 1,
                             S_D->Free_Memory_Slots);
                    printf("\tFree Memory Slots   %d\n",
                            (int) strtol(buf, NULL, 16));
                    printf("<<<<<<<<<<<<<<<<<<<< RX <<<<<<<<<<<<<<<<<<<<\n");
                    break;
                }
            case 's':
                {
                    struct s_Header_Data *s_H_D = (struct s_Header_Data*)md;
                    unsigned char *tmp;
                    float fval;
                    uint32_t val, hours, mins;
                    uint16_t ws, idx, s;

                    printf(">>>>>>>>>>>>>>>>>>>> TX >>>>>>>>>>>>>>>>>>>>\n");
                    memcpy(buf, s_H_D->Base_String, sizeof(s_H_D->Base_String));
                    idx = base_string_index(buf);
                    printf("\tSet                 %s\n", bs_name[idx]);

                    tmp = s_H_D->RF_Address;
                    printf("\tRF address          %x%x%x\n",
                           tmp[0], tmp[1], tmp[2]);

                    val = s_H_D->Room_Nr[0];
                    printf("\tRoom Nr             %d\n", val);

                    switch (idx)
                    {
                        case ProgramData:
                        {
                            struct s_Program_Data *s_P_D =
                                (struct s_Program_Data*)md;
                            val = s_P_D->Day_of_week[0];
                            printf("\tDay Program         %s\n",
                                   week_days[val]);
        
                            s = 0;
                            while (s < MAX_CMD_SETPOINTS * 2)
                            {
                                fval = (s_P_D->Temp_and_Time[s] >> 1) / 2.;
                                ws = (((s_P_D->Temp_and_Time[s] & 1) << 8) |
                                        s_P_D->Temp_and_Time[s + 1]) * 5;
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
                            break;
                        }
                        case EcoModeTemperature:
                        {
                            struct s_Eco_Temp_Data *s_E_D =
                                (struct s_Eco_Temp_Data*)md;
                            fval = s_E_D->Temperature_Comfort[0] / 2.;
                            printf("\tComfort Temperature %.1f\n", fval);
                            fval = s_E_D->Temperature_Eco[0] / 2.;
                            printf("\tEco Temperature     %.1f\n", fval);
                            fval = s_E_D->Temperature_Max[0] / 2.;
                            printf("\tMax Set Point Temp  %.1f\n", fval);
                            fval = s_E_D->Temperature_Min[0] / 2.;
                            printf("\tMin Set Point Temp  %.1f\n", fval);
                            fval = s_E_D->Temperature_Offset[0] / 2. - 3.5;
                            printf("\tTemperature offset  %.1f\n", fval);
                            fval = s_E_D->Temperature_Window_Open[0] / 2.;
                            printf("\tWindow Open Temp    %.1f\n", fval);
                            val = s_E_D->Duration_Window_Open[0] * 5;
                            printf("\tWindow Open Durat   %d\n", val);
                            break;
                        }
                        default:
                            break;
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

/* Log device list info */
void logMAXHostDeviceList(FILE *fp, MAX_msg_list* msg_list)
{
    while (msg_list != NULL) {
        char* md = msg_list->MAX_msg->data;
        switch (msg_list->MAX_msg->type)
        {
            case 'L':
                {
                    struct L_Data *L_D;
                    unsigned char *tmp;
                    int val, len, pos, tlen;
                    float fval;
                    size_t hdr_sz = sizeof(struct MAX_message) - 1;
                    
                    pos = 0;
                    tlen = msg_list->MAX_msg_len - hdr_sz;
                    while (1)
                    {
                        char l_end[] = {0xce, 0x00};
                        int mode;

                        if (memcmp(md + pos, l_end, sizeof(l_end)) == 0)
                        {
                            /* Found terminator, exit */
                            break;
                        }
                        L_D = (struct L_Data*)(md + pos);
                        val = L_D->Submessage_Length[0];
                        len = val;
                        tmp = L_D->RF_Address;
                        /* RF Address */
                        fprintf(fp, "%x%x%x  ",
                               tmp[0], tmp[1], tmp[2]);
                        val = L_D->Flags[0];
                        val = (val << 8) + L_D->Flags[1];
                        mode = val & 0b00000011;
                        if (len > 6)
                        {
                            /* More info available */
                            val = L_D->Valve_Position[0];
                            /* Valve position */
                            fprintf(fp, "%d  ", val);
                            fval = L_D->Temperature[0] / 2.;
                            /* Temperature set */
                            fprintf(fp, "%.1f  ", fval);
                            if (mode == AutoTempMode)
                            {
                                val = L_D->next_data[0] & 0b00000001;
                                val = (val << 8) + L_D->next_data[1];
                                fval = val / 10.;
                                /* Actual temperature */
                                fprintf(fp, "%.1f", fval);
                            }
                            else
                            {
                                fprintf(fp, "NA");
                            }    
                        }
                        else
                        {
                            fprintf(fp, "NA  NA");
                        }    
                        pos += len + 1;
                        fprintf(fp, "\n");
                        if (pos >= tlen)
                        {
                            break;
                        }
                    }
                    break;
                }
            default:
                break;
        }
        msg_list = msg_list->next;
    }
}

unsigned char* findMAXDaySchedule(uint16_t day, MAX_msg_list *msg_list)
{

    if (msg_list && msg_list->MAX_msg->type == 'C')
    {
        char* md = msg_list->MAX_msg->data;
        union C_Data_Device *data =
            (union C_Data_Device*)(md + sizeof(struct C_Data));

        if (data->device.Device_Type[0] == RadiatorThermostat)
        {
            int msg_day, hours;
            uint16_t ws;
            int s;
            union C_Data_Config *config =
                    (union C_Data_Config*)((char*)data +
                    sizeof(union C_Data_Device));

            s = 0;
            msg_day = 0;
            while (s < sizeof(config->rtc.Weekly_Program))
            {
                if (msg_day == day)
                {
                    return &config->rtc.Weekly_Program[s];
                }
                ws = (((config->rtc.Weekly_Program[s] & 1) << 8)
                     | config->rtc.Weekly_Program[s + 1]) * 5;
                hours = ws / 60;
                if (hours >= 24)
                {
                    s = (s / (MAX_DAY_SETPOINTS * 2) + 1) *
                        MAX_DAY_SETPOINTS* 2;
                    msg_day++;
                }
                else
                {
                    s += 2;
                }
            }
        }
    }
    return NULL;
}

MAX_msg_list* findMAXConfig(uint32_t rf_address, MAX_msg_list *msg_list)
{
    char buf[1024];

    while (msg_list != NULL) {
        if (msg_list->MAX_msg && msg_list->MAX_msg->type == 'C')
        {
            char* md = msg_list->MAX_msg->data;
            /* Check if this message is for our device */
            struct C_Data *C_D = (struct C_Data*)md;
            uint32_t msg_rf_address;

            snprintf(buf, sizeof(C_D->RF_address) + 1, C_D->RF_address);
            msg_rf_address = strtol(buf, NULL, 16);
            if (rf_address == msg_rf_address)
            {
                return msg_list;
            }
        }
        msg_list = msg_list->next;
    }
    return NULL;
}

int cmpMAXConfigParam(MAX_msg_list *msg_list, int param, void *value)
{
    if (msg_list && msg_list->MAX_msg && msg_list->MAX_msg->type == 'C')
    {
        char* md = msg_list->MAX_msg->data;
        union C_Data_Device *data =
            (union C_Data_Device*)(md + sizeof(struct C_Data));

        switch (param)
        {
            case EcoConfigParam:
            {
                if (data->device.Device_Type[0] == RadiatorThermostat)
                {
                    union C_Data_Config *config =
                        (union C_Data_Config*)((char*)data +
                                sizeof(union C_Data_Device));
                    float fval;
                            
                    fval = config->rtc.Eco_Temperature[0] / 2.;
                    if (*(float*)value == fval)
                    {
                        return 0;
                    }
                    return 1;
                }
                break;
            }
            case ComfortConfigParam:
            {
                if (data->device.Device_Type[0] == RadiatorThermostat)
                {
                    union C_Data_Config *config =
                        (union C_Data_Config*)((char*)data +
                                sizeof(union C_Data_Device));
                    float fval;
                            
                    fval = config->rtc.Comfort_Temperature[0] / 2.;
                    if (*(float*)value == fval)
                    {
                        return 0;
                    }
                    return 1;
                }
                break;
            }
            default:
                break;
        }
    }
    return -1;
}

/* Dump packet in network format */
void dumpMAXNetpkt(MAX_msg_list* msg_list)
{
    char buff[4096];
    MAX_msg_list *tmpmsg_list = NULL;

    while (msg_list != NULL) {
        memcpy(buff, (char*)msg_list->MAX_msg, msg_list->MAX_msg_len);
        parseMAXData(buff, msg_list->MAX_msg_len, &tmpmsg_list);
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
        if (memcmp(bs_code[i].value, base_string, BS_CODE_SZ) == 0)
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
