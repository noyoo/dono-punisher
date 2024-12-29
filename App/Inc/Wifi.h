#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>

extern uint8_t wifiConnected;

void WIFI_Innit(void);
void wifi_start_sta(void);
#endif