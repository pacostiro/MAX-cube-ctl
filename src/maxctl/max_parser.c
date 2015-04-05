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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include "max_parser.h"
#include "maxmsg.h"

static char* week_days[] = {
    "Saturday",
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday"
};

/* Function to iterate a list and apply a callback function on each element */
int walklist(union cfglist *list, int (*callback)(union cfglist*, void*),
             void *param)
{
    struct list_gen *lg = (struct list_gen*)list, *nlg;

    while (lg)
    {
        int res;
        nlg = lg->next;
        res = callback((union cfglist*)lg, param);
        if (res < 0)
        {
            return res;
        }
        lg = nlg;
    }

    return 0;
}

int dump_program(union cfglist *cl, void *param)
{
    struct program *program;

    if (cl == NULL)
    {
        return -1;
    }

    program = &cl->program;
    printf("#            %.1f %d:%02d;\n", program->temperature, program->hour,
           program->minutes);
    return 0;
}

int dump_auto(union cfglist *cl, void *param)
{
    struct auto_schedule *as;
    struct program *program;

    if (cl == NULL)
    {
        return -1;
    }

    as = &cl->auto_schedule;

    if (as->day < 0 || as->day >= sizeof(week_days) / sizeof(week_days[0]))
    {
        return -1;
    }
    else
    {
        printf("#        %s {\n", week_days[as->day]);
        program = as->schedule;
        if (program != NULL)
        {
            walklist((union cfglist*)program, dump_program, NULL);
        }
        else
        {
            printf("#            (not configured)\n");
        } 
        printf("#        };\n");
    }
    return 0;
}

int dump_ruleset(union cfglist *cl, void *param)
{
    struct config *config;
    struct auto_schedule *as;

    if (cl == NULL || cl->ruleset.device_config == NULL)
    {
        return -1;
    }
    config = &cl->ruleset.device_config->config;
    printf("#device %x {\n", cl->ruleset.device_config->rf_address);
    if (config->room_id != NOT_CONFIGURED_UL)
    {
         printf("#    room %d;\n", config->room_id);
         if (config->eco_temp != NOT_CONFIGURED_F)
         {
             printf("#    eco %.1f;\n", config->eco_temp);
         }
         if (config->comfort_temp != NOT_CONFIGURED_F)
         {
             printf("#    comfort %.1f;\n", config->comfort_temp);
         }
         printf("#    auto {\n");
         as = config->auto_schedule;
         if (as != NULL)
         {
             walklist((union cfglist*)as, dump_auto, NULL);
         }
         else
         {
             printf("#        (not configured)\n");
         } 
         printf("#    };\n");
    }
    else
    {
        printf("#    (not configured)\n");
    }
    printf("#}\n");

    return 0;
}

int free_program(union cfglist *cl, void *param)
{
    struct program *program;

    if (cl == NULL)
    {
        return 0;
    }

    program = (struct program*)cl;
    free(program);
    return 0;
}

int free_auto(union cfglist *cl, void *param)
{
    struct auto_schedule *as;
    struct program *program;

    if (cl == NULL)
    {
        return 0;
    }

    as = (struct auto_schedule*)cl;

    program = as->schedule;
    if (program != NULL)
    {
        walklist((union cfglist*)program, free_program, NULL);
    }
    free(as);

    return 0;
}

int free_ruleset(union cfglist *cl, void *param)
{
    struct auto_schedule *as;
    struct ruleset *ruleset;

    if (cl == NULL || cl->ruleset.device_config == NULL)
    {
        return 0;
    }
    ruleset = (struct ruleset*)cl;
    as = ruleset->device_config->config.auto_schedule;
    if (as != NULL)
    {
        walklist((union cfglist*)as, free_auto, NULL);
    }
    free(ruleset);

    return 0;
}

int flag_auto(union cfglist *cl, void *param)
{
    struct auto_schedule *as;
    struct program *program;
    MAX_msg_list* msg_list = (MAX_msg_list*)param;

    if (cl == NULL)
    {
        return -1;
    }

    as = &cl->auto_schedule;

    if (as->day < 0 || as->day >= sizeof(week_days) / sizeof(week_days[0]))
    {
        return -1;
    }
    else
    {
        unsigned char *msg_program = findMAXDaySchedule(as->day, msg_list);
        int s = 0;
        if (msg_program != NULL)
        {
            /* Raise skip flag and clear it if a difference is found */
            as->skip = 1;
        }                 
        program = as->schedule;
        while (program != NULL)
        {
            float fval;
            uint16_t hours, mins;
            uint16_t ws;
            
            /* Get value from MAX message */
            fval = (msg_program[s] >> 1) / 2.;
            ws = (((msg_program[s] & 1) << 8)
                 | msg_program[s + 1]) * 5;
            hours = ws / 60;
            mins = ws %60;
            /* Compare values and clear skip flag if difference found */
            if (program->temperature != fval ||
                program->hour != hours ||
                program->minutes !=mins)
            {
                as->skip = 0;
                break;
            }
            s += 2;
            program = program->next;
            /* Break loop if schedule in MAX message reached end of day */
            if (hours >= 24)
            {
                break;
            }
        }
    }
    return 0;
}

int flag_ruleset(union cfglist *cl, void *param)
{
    struct config *config;
    struct auto_schedule *as;
    uint32_t rf_address;
    MAX_msg_list* msg_list = (MAX_msg_list*)param;
    int res;

    if (cl == NULL || cl->ruleset.device_config == NULL)
    {
        return -1;
    }
    rf_address = cl->ruleset.device_config->rf_address;
    config = &cl->ruleset.device_config->config;
    if (config->room_id != NOT_CONFIGURED_UL)
    {
        msg_list = findMAXConfig(rf_address, msg_list);
        if (msg_list != NULL)
        {
            /* Raise skip flag and clear it if a difference is found */
            config->skip = 1;
            if (config->eco_temp != NOT_CONFIGURED_F)
            {
                res = cmpMAXConfigParam(msg_list, EcoConfigParam,
                                        &config->eco_temp);
                if (res != 0)
                {
                    config->skip = 0;
                    goto auto_schedule;
                }
            }
            if (config->comfort_temp != NOT_CONFIGURED_F)
            {
                res = cmpMAXConfigParam(msg_list, ComfortConfigParam,
                                         &config->comfort_temp);
                if (res != 0)
                {
                    config->skip = 0;
                    goto auto_schedule;
                }
            }
        }
auto_schedule:
        as = config->auto_schedule;
        if (as != NULL)
        {
            walklist((union cfglist*)as, flag_auto, msg_list);
        }
        /* Could continue in a while loop if there would be more 'C'
         * messages for the same device
         * msglist = findMAXConfig(rf_address, msglist->next); */
    }

    return 0;
}

