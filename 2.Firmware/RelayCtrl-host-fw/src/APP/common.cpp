#include "common.h"
#include <WiFi.h>

bool connectWiFi(String ssid, String pass)
{
    Serial.println("");
    Serial.print("WiFi Connecting: ");
    Serial.print(ssid.c_str());
    Serial.print("@");
    Serial.println(pass.c_str());

    WiFi.disconnect();
    WiFi.mode(WIFI_MODE_STA);

    WiFi.begin(ssid.c_str(), pass.c_str());
    WiFi.setAutoConnect(true);

    uint16_t timeout = 0;
    do
    {
        delay(500);
        Serial.print(".");
    } while ((WiFi.status() != WL_CONNECTED) && (++timeout < 20)); // 10s未连接则超时退出

    if (WiFi.status() != WL_CONNECTED)
    {
        log_e("wifi connect failed");
        return false;
    }

    Serial.println("");
    Serial.println("wifi connect succeed!");
    Serial.printf("RSSI: %d \r\n", WiFi.RSSI());
    Serial.printf("MAC: %s \r\n", WiFi.macAddress().c_str());
    Serial.printf("Obtained IP address: ");
    Serial.println(WiFi.localIP());

    return true;
}

int8_t getWifiRSSI(void)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return 0;
    }

    return WiFi.RSSI();
}