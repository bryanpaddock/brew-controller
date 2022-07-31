#include <ESP8266WiFi.h>
#include "secrets.h"
#include "main.h"

WiFiClient espClient;

void setupWifi()
{
    // connect to wifi
    WiFi.hostname("Brewery-Panel");
    WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASS);

    // not in debug mode - nothing more to do here
    if (!DEBUG)
    {
        return;
    }

    // wait for wifi to connect
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    // print the ip
    Serial.print("IP address: ");
    Serial.print(WiFi.localIP());
}