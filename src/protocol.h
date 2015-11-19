
#ifndef _P99LOGIN_COMMUNICATION_H_
#define _P99LOGIN_COMMUNICATION_H_

#include "connection.h"
#include <stdint.h>

void recv_from_local(Connection* con, int len);
void recv_from_remote(Connection* con, int len);

#endif/*_P99LOGIN_COMMUNICATION_H_*/
