
#include "protocol.h"
#include "sequence.h"

uint16_t get_protocol_opcode(Connection* con)
{
    return ToHostShort(*(uint16_t*)con->buffer);
}

#if _DEBUG
#include <ctype.h>
void debug_write_packet(Connection* con, int len)
{
    int i = 0;
    int j, k;
    uint8_t* buf = con->buffer;

    while ((i + 16) < len)
    {
        j = i;
        i += 16;
        for (k = j; k < i; k++)
            printf("%02x ", buf[k]);
        printf("  ");
        for (k = j; k < i; k++)
            printf("%c", isalnum(buf[k]) ? (char)buf[k] : '.');
        printf("\n");
    }

    if (i == len)
        return;

    for (k = i; k < len; k++)
        printf("%02x ", buf[k]);
    printf("  ");
    j = 16 - (len%16);
    if (j == 16)
        j = 0;
    for (k = 0; k < j; k++)
        printf("   ");
    for (k = i; k < len; k++)
        printf("%c", isalnum(buf[k]) ? (char)buf[k] : '.');
    printf("\n");
}
#endif

void recv_from_local(Connection* con, int len)
{
#if _DEBUG
    printf("CLIENT to LOGIN (len %i):\n", len);
    debug_write_packet(con, len);
#endif

    switch (get_protocol_opcode(con))
    {
    case 0x05: /* OP_SessionDisconnect */
        con->inSession = 0;
        sequence_free(con);
        break;
    case 0x15: /* OP_Ack */
        /* Rewrite client-to-server ack sequence values, since we will be desynchronizing them */
        *(uint16_t*)(&con->buffer[2]) = ToNetworkShort(con->sequence.expectedSequence ? con->sequence.expectedSequence - 1 : 0);
        break;
    default:
        break;
    }

    /* Forward everything */
    connection_send(con, con->buffer, len, 1);
}

void recv_from_remote(Connection* con, int len)
{
#if _DEBUG
    printf("LOGIN to CLIENT (len %i):\n", len);
    debug_write_packet(con, len);
#endif

    switch (get_protocol_opcode(con))
    {
    case 0x02: /* OP_SessionResponse */
        con->inSession = 1;
        sequence_free(con);
        break;
    case 0x09: /* OP_Packet */
        sequence_recv(con, con->buffer, len, 0);
        break;
    case 0x0d: /* OP_Fragment -- must be one of the server list packets */
        sequence_recv(con, con->buffer, len, 1);
        return; /* Don't forward, whole point is to filter this */
    default:
        break;
    }

    /* Forward anything that isn't a fragment */
    connection_send(con, con->buffer, len, 0);
}
