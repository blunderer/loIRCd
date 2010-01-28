/** \file loIRCd.h
 *
 * \author blunderer <blunderer@blunderer.org>
 * \date 28 Jan 2010
 *
 * Copyright (C) 2010 blunderer
 *
 * This binary is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>


#ifndef LOIRCD_H
#define LOIRCD_H

#define CMD_LEN		10
#define MAX_CLIENT	10
#define MAX_CHANS	10

typedef struct _loIRCd_chan loIRCd_chan_t;
typedef struct _loIRCd_client loIRCd_client_t;

struct _loIRCd_chan {
	int id;
	char name[512];
	int clients[MAX_CLIENT];
};

struct _loIRCd_client {
	int id;
	int soc;
	pthread_t service;
	char name[512];
	int chans[MAX_CHANS];
};

/// print CLI usage and exit
void loIRCd_usage(void);

/// parse CLI arguments
int loIRCd_parse_args(int argc, const char ** argv);

/// functions to write and reading sockets
int loIRCd_write_line(int soc, char * buf);
int loIRCd_read_line(int soc, char * cmd, char * buf, int buflen);

/// functions to create join and part chans
int loIRCd_new_chan(char * name);
void loIRCd_join(loIRCd_client_t *self, char *name);
void loIRCd_part(loIRCd_client_t *self, char *buf);

/// function to write a message to chan or user
void loIRCd_talk(char *name, char *buf);

/// function that handle new clients connections
void * loIRCd_new_client(void * t);

/// connect the irc server
int loIRCd_connect(void);

#endif /* LOIRCD_H */

