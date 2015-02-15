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

#ifndef MAX_PARSER_H
#define MAX_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if 1
#define MAX_PARSER_DEBUG
#endif

enum ParamType
{
    RoomId = 0,
    Eco = 1,
    Comfort = 2,
    Auto = 3,
};
#define PARAM_ROOMID  1
#define PARAM_ECO     2
#define PARAM_COMFORT 3
#define PARAM_AUTO    4

struct program {
    float    temperature;
    uint16_t hour;
    uint16_t minutes;
    struct program *next;
};

struct auto_schedule {
    uint16_t day;
    struct program *schedule;
    struct auto_schedule *next;
};

struct config {
    uint32_t room_id;
    float    eco_temp;
    float    comfort_temp;
    struct auto_schedule *auto_schedule;
};

struct devparam {
    int ptype;
    struct config param;
};

struct device_config {
    uint32_t rf_address;
    struct config config;
};

struct ruleset {
    struct device_config *device_config; /* Will be replaced with a union */
    struct ruleset *next;
};

int parse_file(FILE *input);

#endif /* MAX_PARSER_H */
