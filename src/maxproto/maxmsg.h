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

#ifndef MAXMSG_H
#define MAXMSG_H

typedef struct MAX_message {
    char type;
    char colon; /* reserved for ':' */
    char data[1];
};

/* struct H_Data - HEX payload in H message */
struct H_Data {
    char Serial_number[10];
    char comma1; /* reserved for comma */
    char RF_address[6];
    char comma2; /* reserved for comma */
    char Firmware_version[4];
    char comma3; /* reserved for comma */
    char unknown[8];
    char comma4; /* reserved for comma */
    char HTTP_connection_id[8];
    char comma5; /* reserved for comma */
    char Duty_cycle[2];
    char comma6; /* reserved for comma */
    char Free_Memory_Slots[2];
    char comma7; /* reserved for comma */
    char Cube_date[6];
    char comma8; /* reserved for comma */
    char Cube_time[4];
    char comma9; /* reserved for comma */
    char State_Cube_Time[2];
    char comma10; /* reserved for comma */
    char NTP_Counter[4];
    char CRLF[2]; /* reserved for '\n\r' */
};

/* struct C_Data - HEX payload in C message */
struct C_Data {
    char RF_address[6];
    char comma1; /* reserved for comma */
};

/* union C_Data_Device - decoded from Base64 payload in C message */
union C_Data_Device {
    struct cube {
        unsigned char Data_Length[1];
        char Address_of_device[3];
        unsigned char Device_Type[1];
        unsigned char unknown1[1];
        unsigned char Firmware_version[1];
        unsigned char unknown2[1];
        char Serial_Number[10];
    } cube;
    struct device {
        unsigned char Data_Length[1];
        char Address_of_device[3];
        unsigned char Device_Type[1];
        unsigned char Room_ID[1];
        unsigned char Firmware_version[1];
        unsigned char Test_Result[1];
        char Serial_Number[10];
    } device;
};

/* union C_Data_Config - decoded from Base64 payload in C message */
union C_Data_Config {
    struct cubec {
        unsigned char Is_Portal_Enabled[1];
        char Unknown[66];
        char Portal_URL[1];
        /* Unknown */
    } cubec;
    struct rtc {
        unsigned char Comfort_Temperature[1];
        unsigned char Eco_Temperature[1];
        unsigned char Max_Set_Point_Temperature[1];
        unsigned char Min_Set_Point_Temperature[1];
        unsigned char Temperature_offset[1];
        unsigned char Window_Open_Temperature[1];
        unsigned char Window_Open_Duration[1];
        unsigned char Boost[1];
        unsigned char Decalcification[1];
        unsigned char Max_Valve_Setting[1];
        unsigned char Valve_Offset[1];
        unsigned char Weekly_Program[182];
    } rtc;
};

struct m_l {
    char type;
    char colon; /* reserved for ':' */
    char CRLF[2]; /* reserved for '\n\r' */
};

struct m_s {
    char type;
    char colon; /* reserved for ':' */
    char data[1];
};

struct s_Data {
    char Base_String[6];
    char RF_Address[3];
    char Room_Nr[1];
    char Day_of_week[1];
    char Temperature[1];
    char Time_of_day[1];
    char Temperature2[1];
    char Time_of_day2[1];
    char Temperature3[1];
    char Time_of_day3[1];
    char Temperature4[1];
    char Time_of_day4[1];
    char Temperature5[1];
    char Time_of_day5[1];
    char Temperature6[1];
    char Time_of_day6[1];
    char Temperature7[1];
    char Time_of_day7[1];
};

#endif /* MAXMSG_H */
