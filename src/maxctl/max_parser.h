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

#if 0
#define MAX_PARSER_DEBUG
#endif

enum ParamType
{
    RoomId = 0,
    Eco = 1,
    Comfort = 2,
    Auto = 3,
};

/* Used for empty configuration */
#define NOT_CONFIGURED_UL ((uint32_t)0xffffffff)
#define NOT_CONFIGURED_F ((float)0xffffffff)

struct program {
    struct program *next;
    float    temperature;
    uint16_t hour;
    uint16_t minutes;
};

struct auto_schedule {
    struct auto_schedule *next;
    uint16_t day;
    int      skip;
    struct program *schedule;
};

struct config {
    uint32_t room_id;
    float    eco_temp;
    float    comfort_temp;
    int      skip;
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
    struct ruleset *next;
    struct device_config *device_config; /* Will be replaced with a union */
};

/* Generic list */
struct list_gen {
    struct list_gen *next;
    char data[1];
};

union cfglist {
    struct list_gen list_gen;
    struct ruleset ruleset;
    struct auto_schedule auto_schedule;
    struct program program;
};

/* dump_ruleset prints out an entry in the rule set. An entry corresponds to a
 * device */
int dump_ruleset(union cfglist *cl, void *param);
/* flag_ruleset flags an entry in the rule set if it is identical to the
 * configuration found in the message list passed as argument. */
int flag_ruleset(union cfglist *cl, void *param);
/* free_ruleset frees an entry in the rule set. An entry corresponds to a
 * device */
int free_ruleset(union cfglist *cl, void *param);

/* Function to iterate a list and apply a callback function on each element */
int walklist(union cfglist *list, int (*callback)(union cfglist*, void*),
             void *param);

int parse_file(FILE *input, struct ruleset **ruleset);

#endif /* MAX_PARSER_H */
