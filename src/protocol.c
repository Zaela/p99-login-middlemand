
#include "protocol.h"
#include "sequence.h"

uint16_t get_protocol_opcode(uint8_t* data)
{
    return ToHostShort(*(uint16_t*)data);
}

#if _DEBUG
#include <ctype.h>
void debug_write_packet(uint8_t* buf, int len, int loginToClient)
{
    int i = 0;
    int j, k;

    printf("%ld ", time(NULL));
    if (loginToClient)
        printf("LOGIN to CLIENT (len %i):\n", len);
    else
        printf("CLIENT to LOGIN (len %i):\n", len);

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
    switch (get_protocol_opcode(con->buffer))
    {
    case 0x03: /* OP_Combined */
        sequence_adjust_combined(con, len);
        break;
    case 0x05: /* OP_SessionDisconnect */
        con->inSession = 0;
        sequence_free(con);
        break;
    case 0x15: /* OP_Ack */
        /* Rewrite client-to-server ack sequence values, since we will be desynchronizing them */
        sequence_adjust_ack(con, con->buffer, len);
        break;
    default:
        break;
    }

    /* Forward everything */
    connection_send(con, con->buffer, len, 1);
}

void recv_from_remote(Connection* con, uint8_t* data, int len)
{
    switch (get_protocol_opcode(data))
    {
    case 0x02: /* OP_SessionResponse */
        con->inSession = 1;
        sequence_free(con);
        break;
    case 0x03: /* OP_Combined */
        sequence_recv_combined(con, data, len);
        return; /* Pieces will be forwarded individually */
    case 0x09: /* OP_Packet */
        sequence_recv_packet(con, data, len);
        break;
    case 0x0d: /* OP_Fragment -- must be one of the server list packets */
        sequence_recv_fragment(con, data, len);
        return; /* Don't forward, whole point is to filter this */
    default:
        break;
    }

    /* Forward anything that isn't a fragment */
    connection_send(con, data, len, 0);
}
