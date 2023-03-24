#ifndef __COMMON_H
#define __COMMON_H

#include <Arduino.h>

bool connectWiFi(String ssid, String pass);
int8_t getWifiRSSI(void);

#endif