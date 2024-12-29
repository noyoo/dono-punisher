#ifndef APP_H
#define APP_H

typedef enum {
    WifiInnit,
    WaitForWifiInnitComplete,
    SyncTime,
    HttpInnit,
    WaitForHttpInnitComplete,
    Poll,
    Wait
} State_t;

void stateMachine(void);

#endif