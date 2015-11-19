
#ifndef _P99LOGIN_NETCODE_H_
#define _P99LOGIN_NETCODE_H_

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <net/if.h>
#endif

#define ToNetworkShort htons
#define ToNetworkLong htonl
#define ToHostShort ntohs
#define ToHostLong ntohl

#ifndef _WIN32
#define closesocket close
#define INVALID_SOCKET -1
#else
typedef int socklen_t;
#endif

#endif/*_P99LOGIN_NETCODE_H_*/
