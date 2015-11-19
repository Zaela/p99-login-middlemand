
#include "sequence.h"
#include "connection.h"

uint16_t get_sequence(uint8_t* data)
{
    return ToHostShort(*(uint16_t*)(&data[2]));
}

void sequence_free(Connection* con)
{
    Sequence* seq = &con->sequence;
    uint16_t i;
    uint16_t count;

    if (!seq->packets)
        return;

    count = seq->count;
    for (i = 0; i < count; i++)
    {
        if (seq->packets[i].data)
            free(seq->packets[i].data);
    }

    free(seq->packets);
    memset(seq, 0, sizeof(Sequence));
    seq->expectedSequence = 2;
}

void grow(Connection* con, uint32_t index)
{
    Sequence* seq = &con->sequence;
    uint32_t cap = 32;
    Packet* array;

    while (cap <= index)
        cap *= 2;

    array = (Packet*)malloc(cap * sizeof(Packet));
    if (!array)
        longjmp(con->jmpBuf, ERR_BAD_ALLOC);

    //memset(array + seq->capacity, 0, (cap - seq->capacity) * sizeof(Packet));
    memset(array, 0, cap * sizeof(Packet));

    if (seq->capacity != 0)
    {
        memcpy(array, seq->packets, seq->capacity * sizeof(Packet));
        free(seq->packets);
    }

    seq->capacity = cap;
    seq->packets = array;
}

void check_fragment_finished(Connection* con);
int process_first_fragment(Connection* con, uint8_t* data);
void filter_server_list(Connection* con, int totalLen);

void send_ack(Connection* con, int seq)
{
    uint8_t ack[6];

    ack[0] = 0;
    ack[1] = 0x15;
    *(uint16_t*)(&ack[2]) = ToNetworkShort(seq != -1 ? seq : con->sequence.expectedSequence - 1);

    /* Login server uses zero for its CRC key, always produces two zero bytes for CRCs */
    /*ack[4] = 0;
    ack[5] = 0;*/

    connection_send(con, ack, 6, 1);
}

void copy_fragment(Connection* con, Packet* p, uint8_t* data, int len)
{
    uint8_t* copy = (uint8_t*)malloc(len);
    if (!copy)
        longjmp(con->jmpBuf, ERR_BAD_ALLOC);
    memcpy(copy, data, len);

    p->data = copy;
}

void sequence_recv(Connection* con, uint8_t* data, int len, int isFragment)
{
    Sequence* seq = &con->sequence;
    uint16_t val = get_sequence(data);
    uint8_t* copy;
    uint32_t index;
    Packet* p;

    send_ack(con, val);

    index = val;// - seq->firstSequence;
    if (val >= seq->count)
        seq->count = val + 1;

    if (index >= seq->capacity)
    {
        if (seq->capacity == 0)
            seq->firstSequence = val;
        grow(con, index);
    }

    p = &seq->packets[index];
    if (p->data)
        free(p->data);

    p->isFragment = isFragment;
    p->data = NULL;
    p->len = len;// - 2; /* Ignore CRC footer */

    /* Not the "next" packet? */
    if (seq->expectedSequence != val)
    {
        /* Could be the packet that completes the fragment sequence */
        if (isFragment)
        {
            copy_fragment(con, p, data, len);
            check_fragment_finished(con);
        }

        return;
    }

    /* This is the first fragment in a fragment sequence */
    if (isFragment && process_first_fragment(con, data))
    {
        seq->fragStart = index;
        copy_fragment(con, p, data, len);
        return;
    }

    seq->expectedSequence++;
}

/* First fragment: ProtocolOpcode[2] Sequence[2] TotalLength[4] AppOpcode[2] data[...] CRC[2] */
#pragma pack(1)
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

int process_first_fragment(Connection* con, uint8_t* data)
{
    FirstFrag* frag = (FirstFrag*)data;

    if (frag->appOpcode != 0x18)
        return 0;

    con->sequence.fragmentLen = ToHostLong(frag->totalLen);
    return 1;
}

void check_fragment_finished(Connection* con)
{
    Sequence* seq = &con->sequence;
    uint32_t index = seq->fragStart;
    int got;
    Packet* p;
    int n = seq->fragmentLen / 512;
    int count = 0;

    p = &seq->packets[index];
    got = p->len - sizeof(FirstFrag) + 2; /* AppOpcode is counted */

    //while (got < seq->fragmentLen)
    while (count < n)
    {
        index++;
        if (index >= seq->count)
            return;

        p = &seq->packets[index];
        if (!p->data)
            return;

        got += p->len - sizeof(Frag);
        count++;
    }

    /* If we reach here, we had the whole sequence! */
    filter_server_list(con, got - 2); /* Don't count AppOpcode here */
}

int compare_prefix(const char* a, const char* b, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        if (*a++ != *b++)
            return 0;
    }

    return 1;
}

void filter_server_list(Connection* con, int totalLen)
{
    Sequence* seq = &con->sequence;
    uint32_t index = seq->fragStart;
    int pos, i, outCount;
    Packet* p;
    uint8_t* serverList;
    uint8_t outBuffer[512]; /* Should not need nearly this much space just for P99 server listings */
    int outLen = 0;
    char* name;

    p = &seq->packets[index];
    if (p->len == 0)
        return;

    serverList = (uint8_t*)malloc(totalLen);

    if (!serverList)
        longjmp(con->jmpBuf, ERR_BAD_ALLOC);

    memcpy(serverList, p->data + sizeof(FirstFrag), p->len - sizeof(FirstFrag));
    pos = p->len - sizeof(FirstFrag); /* Not counting AppOpcode */

    while (pos < totalLen)
    {
        index++;
        p = &seq->packets[index];

        memcpy(serverList + pos, p->data + sizeof(Frag), p->len - sizeof(Frag));
        pos += p->len - sizeof(Frag);
    }

    /* We now have the whole server list in one piece */
    /* Write our output packet header */
    outBuffer[0] = 0;
    outBuffer[1] = 0x09; /* OP_Packet */
    outBuffer[2] = 0;
    outBuffer[3] = (uint8_t)seq->expectedSequence;
    outBuffer[4] = 0x18; /* OP_ServerListResponse */
    outBuffer[5] = 0;

    /* First 16 bytes of the server list packet is some kind of header, copy it over */
    for (i = 0; i < 16; i++)
        outBuffer[i + 6] = serverList[i];
    
    /* outBuffer[22] is a 4-byte count of the number of servers on the list -- we don't know the value yet, probably 2 */
    outLen = 16 + 6 + 4;
    outCount = 0;

    /* List of servers starts at serverList[20] */
    pos = 20;

    while (pos < totalLen)
    {
        /* Server listings are variable-size */
        i = pos; /* Start of this server in serverList */
        pos += strlen((char*)(serverList + pos)) + 1; /* IP address */
        pos += sizeof(int) * 2; /* ListId, runtimeId */
        // should we change listId? probably
        name = (char*)(serverList + pos);
        pos += strlen(name) + 1;
        pos += strlen((char*)(serverList + pos)) + 1; /* language */
        pos += strlen((char*)(serverList + pos)) + 1; /* region */
        pos += sizeof(int) * 2; /* Status, player count */

        /* Time to check the name! */
        if (compare_prefix(name, SERVER_NAME_PREFIX, sizeof(SERVER_NAME_PREFIX) - 1))
        {
            outCount++;
            memcpy(outBuffer + outLen, serverList + i, pos - i);
            outLen += pos - i;
        }
    }

    /* Write our outgoing server count */
    *(int*)(&outBuffer[22]) = outCount;

    /* Login server uses zero for its CRC key, always produces two zero bytes for CRCs */
    /*outBuffer[outLen] = 0;
    outBuffer[outLen + 1] = 0;
    outLen += 2;*/

    seq->expectedSequence += index + 1 - seq->fragStart;
    free(serverList);

    connection_send(con, outBuffer, outLen, 0);

    /* Send the remote side an ack for their last fragment */
    send_ack(con, -1);

    seq->sentServerList = 1;
}
