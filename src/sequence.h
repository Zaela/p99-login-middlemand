
#ifndef _P99LOGIN_SEQUENCE_H_
#define _P99LOGIN_SEQUENCE_H_

#include "netcode.h"
#include <stdint.h>

#define SERVER_NAME_PREFIX "Project 1999"

typedef struct Connection Connection;

typedef struct Packet {
    int isFragment;
    int len;
    uint8_t* data;
} Packet;

typedef struct Sequence {
    Packet* packets;
    uint32_t capacity;
    uint32_t count;
    uint16_t firstSequence;
    uint16_t expectedSequence;
    uint32_t fragStart;
    int fragmentLen;
    int sentServerList;
} Sequence;

void sequence_free(Connection* con);
void sequence_recv(Connection* con, uint8_t* data, int len, int isFragment);

#endif//_P99LOGIN_SEQUENCE_H_
