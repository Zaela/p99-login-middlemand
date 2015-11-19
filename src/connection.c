
#include "connection.h"
#include "protocol.h"

void connection_open(Connection* con)
{
    Address addr;
    struct addrinfo hints;
    struct addrinfo* remote;

    /* Look up the login server IP */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(REMOTE_HOST, REMOTE_PORT, &hints, &remote))
        longjmp(con->jmpBuf, ERR_GETADDRINFO_CALL);

    con->remoteAddr = *(Address*)remote->ai_addr;

    freeaddrinfo(remote);

    /* Create our UDP socket */
    con->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (con->socket == INVALID_SOCKET)
        longjmp(con->jmpBuf, ERR_SOCKET_CALL);

    memset(&addr, 0, sizeof(Address));
    addr.sin_family = AF_INET;
    addr.sin_port = ToNetworkShort(MIDDLEMAN_PORT);
    addr.sin_addr.s_addr = ToNetworkLong(INADDR_ANY);

    if (bind(con->socket, (struct sockaddr*)&addr, sizeof(Address)))
        longjmp(con->jmpBuf, ERR_BIND_CALL);

    con->inSession = 0;
    con->lastRecvTime = 0;
    memset(&con->sequence, 0, sizeof(Sequence));
    con->sequence.expectedSequence = 2;
}

void connection_close(Connection* con)
{
    if (con->socket != INVALID_SOCKET)
    {
        closesocket(con->socket);
        con->socket = INVALID_SOCKET;
    }

    sequence_free(con);

#ifdef _WIN32
    WSACleanup();
#endif
}

void connection_read(Connection* con)
{
    Address addr;
    int addrLen = sizeof(Address);
    int len;
    time_t recvTime;

    len = recvfrom(con->socket, (char*)con->buffer, BUFFER_SIZE, 0, (struct sockaddr*)&addr, &addrLen);

    /* Anything less than 4 bytes is not meaningful to us */
    if (len < 4)
    {
        if (len == -1)
            longjmp(con->jmpBuf, ERR_RECVFROM);
        return;
    }

    recvTime = time(NULL);

    /* Is this packet from the remote login server? */
    if (addr.sin_addr.s_addr == con->remoteAddr.sin_addr.s_addr && addr.sin_port == con->remoteAddr.sin_port)
    {
        recv_from_remote(con, len);
    }
    else
    {
        /* If this isn't from the login server and we weren't in a session, record the address we received from */
        if (!con->inSession || (recvTime - con->lastRecvTime) > 10)
            connection_reset(con, &addr);

        recv_from_local(con, len);
    }

    con->lastRecvTime = recvTime;
}

void connection_send(Connection* con, void* data, int len, int toRemote)
{
    Address* addr;
    int sent;

    if (toRemote)
        addr = &con->remoteAddr;
    else
        addr = &con->localAddr;

    sent = sendto(con->socket, (char*)data, len, 0, (struct sockaddr*)addr, sizeof(Address));

    if (sent == -1)
        longjmp(con->jmpBuf, ERR_SENDTO);
}

void connection_reset(Connection* con, Address* addr)
{
    con->localAddr = *addr;
    sequence_free(con);
}
