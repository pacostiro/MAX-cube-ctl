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

#ifndef MAX_H
#define MAX_H

#include "maxmsg.h"

#define MSG_END "\r\n" /* Message terminator sequence */
#define MSG_END_LEN 2  /* Message terminator sequence len */

#define MAX_DISCOVER_PORT 23272
#define MAX_TCP_PORT 62910

#define MAX_MCAST_ADDR "224.0.0.1"
#define MAX_BCAST_ADDR "255.255.255.255"

/* MAXDiscover retrieves the IP address of a cube in the LAN */
/* Return value: negative if an error has occured, zero if no cube was found,
 * positive if a cube has been found */
int MAXDiscover(struct sockaddr *sa, socklen_t sa_len,
    struct Discover_Data *D_Data, int tmo);
int MAXConnect(struct sockaddr *sa);
int MAXDisconnect(int connectionId);
int MAXMsgSend(int connectionId, MAX_msg_list *output_msg_list);
int MaxMsgRecv(int connectionId, MAX_msg_list **input_msg_list);
int MaxMsgRecvTmo(int connectionId, MAX_msg_list **input_msg_list, int tmo);

#endif /* MAX_H */
