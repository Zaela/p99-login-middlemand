
#ifndef _P99LOGIN_CONNECTION_H_
#define _P99LOGIN_CONNECTION_H_

#include "netcode.h"
#include "sequence.h"
#include "errors.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>
#include <time.h>

#ifndef MIDDLEMAN_PORT
#define MIDDLEMAN_PORT 5998
#endif

#ifndef REMOTE_HOST
#define REMOTE_HOST "login.eqemulator.net"
#endif

#ifndef REMOTE_PORT
#define REMOTE_PORT "5998"
#endif

#define BUFFER_SIZE 2048
#define SESSION_TIMEOUT_SECONDS 60

typedef struct sockaddr_in Address;

typedef struct Connection {
    int socket;
    int inSession;
    time_t lastRecvTime;
    Address localAddr;
    Address remoteAddr;
    jmp_buf jmpBuf;
    uint8_t buffer[BUFFER_SIZE];
    Sequence sequence;
} Connection;

void connection_open(Connection* con);
void connection_close(Connection* con);
void connection_read(Connection* con);
void connection_send(Connection* con, void* data, int len, int toRemote);
void connection_reset(Connection* con, Address* addr);

#endif/*_P99LOGIN_CONNECTION_H_*/