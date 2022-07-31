#include <ArduinoHA.h>
#include <Ethernet.h>
#include "main.h"
#include "probes.h"
#include "wifi.h"
#include "secrets.h"

byte mac[] = {0xe8, 0xdb, 0x84, 0xe1, 0x14, 0x32};

HADevice device(mac, sizeof(mac));
HAMqtt mqtt(espClient, device);

// unique_id, device class, default value
HABinarySensor panelActiveSensor("control_panel.active", "panel", true);
HASensor sensorHltTemperatureCurrent("control_panel.hot_liqour_tank.temperature");
HASensor sensorHltTemperatureAlarm("control_panel.hot_liqour_tank.temperature_alarm");
HABinarySensor sensorHltHeating("control_panel.hot_liqour_tank.heating", "panel", false);
HABinarySensor sensorHltAlarmEnabled("control_panel.hot_liqour_tank.alarm.enabled", "panel", false);

HABinarySensor sensorAlarmBuzzing("control_panel.alarm.buzzing", "panel", false);

// alarms
bool alarmEnabled = true;
bool alarmBuzzing = false;

void onBeforeSwitchStateChanged(bool state, HASwitch *s)
{
    // this callback will be called before publishing new state to HA
    // in some cases there may be delay before onStateChanged is called due to network latency
}

void onSwitchStateChanged(bool state, HASwitch *s)
{
}

void setupMqttMain()
{
    // set up main active sensor
    panelActiveSensor.setName("Brewery Control Panel Online");

    // is the alarm currently buzzing
    sensorAlarmBuzzing.setName("Alarm Buzzing");
}

void setupMqttHotLiqourTank()
{
    // binary sensor
    sensorHltHeating.setName("Hot Liquor Tank Heating");

    // binary sensor
    sensorHltAlarmEnabled.setName("Hot Liquor Tank Alarm Enabled");

    // sensor
    sensorHltTemperatureCurrent.setIcon("mdi:thermometer");
    sensorHltTemperatureCurrent.setName("Hot Liquor Tank Current Temperature");

    // sensor
    sensorHltTemperatureAlarm.setIcon("mdi:thermometer");
    sensorHltTemperatureAlarm.setName("Hot Liquor Tank Alarm Temperature");
}

void alarmBuzzOn()
{
    alarmBuzzing = true;
}

void alarmBuzzOff()
{
    alarmBuzzing = false;
}

void sendData()
{
    panelActiveSensor.setState(true);
    sensorAlarmBuzzing.setState(alarmBuzzing);

    // hot liquor tank --------------------

    // activity
    sensorHltHeating.setState(false);

    // temps
    sensorHltTemperatureCurrent.setValue(HLTaverage);
    sensorHltTemperatureAlarm.setValue(HLTSetPoint);
    sensorHltAlarmEnabled.setState(alarmEnabled);
}

// -------------------------------------------------------------------------------------------

void setupMqtt()
{
    // set device's details (optional)
    device.setName("Brewery");
    device.setSoftwareVersion(version);

    setupMqttMain();
    setupMqttHotLiqourTank();

    mqtt.begin(SECRET_MQTT_SERVER);
}

void loopMqtt()
{
    mqtt.loop();

    sendData();
}