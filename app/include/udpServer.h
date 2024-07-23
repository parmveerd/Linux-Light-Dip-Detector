#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_

typedef enum {
    HELP,
    COUNT,
    LENGTH,
    DIPS,
    HISTORY,
    STOP,
    UNKNOWN
} ServerCommands;

int initUdpServer(int*);
int shutdownUdpServer();
void bWaitMainThread();

#endif 
