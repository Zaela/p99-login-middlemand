
#include "connection.h"

int print_error(int errcode);

int main()
{
    Connection con;
    int exception;
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa))
        return print_error(ERR_WSASTARTUP);
#endif

    exception = setjmp(con.jmpBuf);

    if (exception)
    {
        connection_close(&con);
        return print_error(exception);
    }

    connection_open(&con);

    for (;;)
    {
        /* This always blocks */
        connection_read(&con);
    }

    connection_close(&con);

    return 0;
}

int print_error(int errcode)
{
    printf("Error: %d\n", errcode);
    return errcode;
}
