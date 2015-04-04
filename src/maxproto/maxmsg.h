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
 *
 * The MIT License (MIT)
 * Copyright (c) 2013 bouni
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef MAXMSG_H
#define MAXMSG_H

enum MaxDeviceType
{
    Cube = 0,
    RadiatorThermostat = 1,
    RadiatorThermostatPlus = 2,
    WallThermostat = 3,
    ShutterContact = 4,
    EcoButton = 5
};

enum MaxConfigParam
{
    EcoConfigParam = 0,
    ComfortConfigParam = 1
};

/* Size of the code in chars */
#define BS_CODE_SZ  6

enum BSIndex
{
    TemperatureAndMode = 0,
    ProgramData = 1,
    EcoModeTemperature = 2,
    ConfigValveFunctions = 3,
    AddLinkPartner = 4,
    RemoveLinkPartner = 5,
    SetGroupAddress =6
};

/* struct MAX_message is a generic structure to be used for all messages. */
struct MAX_message {
    char type;    /* message name */
    char colon;   /* reserved for ':' */
    char data[1]; /* data points to beginning of message payload */
};

/* MAX_msg_list is structure used to implement a list of messages that are
 * part of a packet */
typedef struct MME
{
    struct MME *prev;
    struct MME *next;
    size_t MAX_msg_len;
    struct MAX_message *MAX_msg;
} MAX_msg_list;

/* Message definitions
 * ==========================================
 * Data structures describe payload of messages in host format. Some of the
 * messages are base64 encoded when sent over the network. Decoded messages do
 * not contain the trailing '\r\n' anymore and neither separating comas.
 */

/*
 * Incoming messages
 * ==========================================
 * INCOMING HELLO:                       "H:"
 * INCOMING NTP SERVER:                  "F:"
 * INCOMING DEVICE LIST:                 "L:"
 * INCOMING CONFIGURATION:               "C:"
 * INCOMING METADATA:                    "M:"
 * INCOMING NEW DEVICE:                  "N:"
 * INCOMING ACKNOWLEDGE:                 "A:"
 * INCOMING ENCRYPTION:                  "E:"
 * INCOMING DECRYPTION:                  "D:"
 * INCOMING SET CREDENTIALS:             "b:"
 * INCOMING GET CREDENTIALS:             "g:"
 * INCOMING SET REMOTEACCESS:            "j:"
 * INCOMING SET USER DATA:               "p:"
 * INCOMING GET USER DATA:               "o:"
 * INCOMING CHECK PRODUCT ACTIVATION:    "v:"
 * INCOMING ACTIVATE PRODUCT:            "w:"
 * INCOMING SEND DEVICE CMD:             "S:"
 */

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
        unsigned char Address_of_device[3];
        unsigned char Device_Type[1];
        unsigned char unknown1[1];
        unsigned char Firmware_version[1];
        unsigned char unknown2[1];
        char Serial_Number[10];
    } cube;
    struct device {
        unsigned char Data_Length[1];
        unsigned char Address_of_device[3];
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

/* struct S_Data - HEX payload in S message */
struct S_Data {
    char Duty_Cycle[2];
    char comma1; /* reserved for comma */
    char Command_Result[1];
    char comma2; /* reserved for comma */
    char Free_Memory_Slots[2];
    char CRLF[2]; /* reserved for '\n\r' */
};

/* struct L_Data - decoded from Base64 payload in L message */
struct L_Data {
    char Submessage_Length[1];
    unsigned char RF_Address[3];
    char Unknown[1];
    unsigned char Flags[2];
    /* If the submessage length is greater than 6, these fields follow */
    unsigned char Valve_Position[1];
    unsigned char Temperature[1];
    /* generic pointer to following data */
    unsigned char next_data[1];
};

/* struct Discover_Data - HEX payload in Discover reply */
struct Discover_Data {
    char Name[8];
    char Serial_number[10];
    unsigned char Request_ID[1];
    char Request_Type[1];
    char Reserved[1]; /* Reserved for NULL terminator */
    unsigned char RF_address[3];
    unsigned char Firmware_version[2];
};

/* Outgoing messages
 * ==========================================
 * OUTGOING URL:                         "u:"
 * OUTGOING INTERVAL:                    "i:"
 * OUTGOING SEND:                        "s:"
 * OUTGOING METADATA:                    "m:"
 * OUTGOING INCLUSION MODE:              "n:"
 * OUTGOING CANCEL INCLUSION MODE:       "x:"
 * OUTGOING MORE DATA:                   "g:"
 * OUTGOING QUIT:                        "q:"
 * OUTGOING ENCRYPTION:                  "e:"
 * OUTGOING DECRYPTION:                  "d:"
 * OUTGOING SET CREDENTIALS:             "B:"
 * OUTGOING GET CREDENTIALS:             "G:"
 * OUTGOING SET REMOTEACCESS:            "J:"
 * OUTGOING SET USER DATA:               "P:"
 * OUTGOING GET USER DATA:               "O:"
 * OUTGOING CHECK PRODUCT ACTIVATION:    "V:"
 * OUTGOING ACTIVATE PRODUCT:            "W:"
 * OUTGOING SEND DEVICE CMD:             "s:"
 * OUTGOING RESET:                       "a:"
 * OUTGOING RESET ERROR:                 "r:"
 * OUTGOING DELETE DEVICES:              "t:"
 * OUTGOING SET PUSHBUTTON CONFIG:       "w:"
 * OUTGOING GET DEVICE LIST:             "l:"
 * OUTGOING SET URL:                     "u:"
 * OUTGOING GET CONFIGURATION:           "c:"
 * OUTGOING TIME CONFIG:                 "v:"
 * OUTGOING NTP SERVER:                  "f:"
 * OUTGOING SEND WAKEUP:                 "z:"
*/

/* Maximum number of set points per s command */
#define MAX_CMD_SETPOINTS 7

/*
 * 's' command data messages
 *
 * "temperature and mode"
 * "program data"
 * "eco mode temperature"
 * "config valve functions"
 * "add link partner"
 * "remove link partner"
 * "set group address"
 */

/* struct s_Header_Data - HEX payload in s message, header part */
struct s_Header_Data {
    char Base_String[6];
    unsigned char RF_Address[3];
    unsigned char Room_Nr[1];
};

/* struct s_Program_Data - HEX payload in s message for 'program data' */
struct s_Program_Data {
    char Base_String[6];
    unsigned char RF_Address[3];
    unsigned char Room_Nr[1];
    unsigned char Day_of_week[1];
    unsigned char Temp_and_Time[2];
    unsigned char Temp_and_Time2[2];
    unsigned char Temp_and_Time3[2];
    unsigned char Temp_and_Time4[2];
    unsigned char Temp_and_Time5[2];
    unsigned char Temp_and_Time6[2];
    unsigned char Temp_and_Time7[2];
};

enum TempMode
{
    AutoTempMode = 0,
    ManualTempMode = 1,
    VacationTempMode = 2,
    BoostMode = 3
};

/* struct s_Temp_Mode_Data - HEX payload in s message for
 * 'temperature and mode' */
struct s_Temp_Mode_Data {
    char Base_String[6];
    unsigned char RF_Address[3];
    unsigned char Room_Nr[1];
    unsigned char Temp_and_Mode[1];
    unsigned char Date_Until[1];
};

/* struct s_Eco_Temp_Data - HEX payload in s message for
 *  * 'eco/comfort/etc temperature' */
struct s_Eco_Temp_Data {
    char Base_String[6];
    unsigned char RF_Address[3];
    unsigned char Room_Nr[1];
    unsigned char Temperature_Comfort[1];
    unsigned char Temperature_Eco[1];
    unsigned char Temperature_Max[1];
    unsigned char Temperature_Min[1];
    unsigned char Temperature_Offset[1];
    unsigned char Temperature_Window_Open[1];
    unsigned char Duration_Window_Open[1];
};

/* struct q_Data - contains only CR LF terminator for q message */
struct q_Data {
    char CRLF[2]; /* reserved for '\n\r' */
};

/* struct l_Data - contains only CR LF terminator for l message */
struct l_Data {
    char CRLF[2]; /* reserved for '\n\r' */
};

/* Dump packet in host format */
void dumpMAXHostpkt(MAX_msg_list* msg_list);
/* Dump packet in network format */
void dumpMAXNetpkt(MAX_msg_list* msg_list);
/* Free all elements in a message list */
void freeMAXpkt(MAX_msg_list **msg_list);
/* Return a pointer to the day schedule in the message */
unsigned char* findMAXDaySchedule(uint16_t day, MAX_msg_list *msg_list);
/* find 'C' message in a message list that corresponds to a device by
 * rf_address */
MAX_msg_list* findMAXConfig(uint32_t rf_address, MAX_msg_list *msg_list);
/* Compare config parameter with the one from a message.
 * Return '0' if the same 'value' is found in the message list */
int cmpMAXConfigParam(MAX_msg_list *msg_list, int param, void *value);

/* Appends a message 'msg' to a packet (list of messages) msg_list. Return value
 * points to the first element in the list. msg_list can be NULL, in that case
 * there will be one element in the list after calling this function */
MAX_msg_list* appendMAXmsg(MAX_msg_list* msg_list, struct MAX_message *msg,
    size_t msg_len);

/* Returns the index of the given base string */
int base_string_index(const char *base_string);
/* Returns a string representing the base string situated at 'index' position in
 * the base string array */
const char*  base_string_code(int index);

#endif /* MAXMSG_H */
