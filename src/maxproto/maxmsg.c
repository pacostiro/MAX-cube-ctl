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

#define MAX_SCHED 13

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

