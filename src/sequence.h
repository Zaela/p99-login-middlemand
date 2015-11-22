
#ifndef _P99LOGIN_SEQUENCE_H_
#define _P99LOGIN_SEQUENCE_H_

#include "netcode.h"
#include <stdint.h>
#include <stdlib.h>

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
    uint32_t fragStart;
    int fragCount;
    uint16_t seqToLocal;    /* What the client thinks the next sequence from the login server should be */
    uint16_t seqFromRemote; /* The next "real" sequence we expect from the login server */
} Sequence;

#pragma pack(1)
/* First fragment: ProtocolOpcode[2] Sequence[2] TotalLength[4] AppOpcode[2] data[...] CRC[2] */
typedef struct FirstFrag {
    uint16_t protocolOpcode;
    uint16_t sequence;
    uint32_t totalLen;
    uint16_t appOpcode;
} FirstFrag;

/* Subsequent packets: ProtocolOpcode[2] Sequence[2] data[...] CRC[2] */
typedef struct Frag {
    uint16_t protocolOpcode;
    uint16_t sequence;
} Frag;
#pragma pack()

void sequence_init(Connection* con);
void sequence_free(Connection* con);
void sequence_recv(Connection* con, uint8_t* data, int len, int isFragment);

void sequence_recv_packet(Connection* con, uint8_t* data, int len);
void sequence_recv_fragment(Connection* con, uint8_t* data, int len);
void sequence_recv_combined(Connection* con, uint8_t* data, int len);
void sequence_adjust_ack(Connection* con, uint8_t* ack, int len);
void sequence_adjust_combined(Connection* con, int len);

#endif/*_P99LOGIN_SEQUENCE_H_*/
