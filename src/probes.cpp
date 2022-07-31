#include <OneWire.h>
#include <DallasTemperature.h>
#include "main.h"

// pins
int pinOneWireBus = 0;

// set up the actual probes now
DeviceAddress probeHLT = {0x28, 0xFF, 0xBF, 0x3D, 0x64, 0x14, 0x04, 0x30};
DeviceAddress probeInternalTemp = {0x28, 0x06, 0x36, 0x6A, 0x05, 0x00, 0x00, 0x0A};

// set up onewire bus
OneWire oneWire(pinOneWireBus);

// send onewire to dallas temperature lib
DallasTemperature sensors(&oneWire);

// set temp probe resolution.
const int resolution = 9;

// ds18b20 takes time to respond. bypass that with a timer.
unsigned long lastTempRequest;

// the actual delay is calculated in setup() based on resolution
unsigned long tempDelay;

bool hasFoundInternalProbe = false;
int hasFoundHltProbe = false;

// smooth the temperature so the ssr
// doesnt switch too much while hovering
// around the Setpoint
const int numReadings = 10;
int HLTreadings[numReadings]; // the readings from the probe
int HLTindex = 0;             // the index of the current reading
int HLTtotal = 0;             // the running total
int HLTaverage = 0;           // the average
int HLTgotFirstAverage = 0;   // only print temp once we have enough values
int HLTSetPoint = 70;         // HLT temp setpoint

int tempInternal = 0;

int deviceCount = 0;

float readTemperature(DeviceAddress deviceAddress)
{
    float tempC = sensors.getTempC(deviceAddress);
    if (tempC == -127.00)
    {
        if (DEBUG)
        {
            Serial.println("Error getting temperature");
        }
    }

    return tempC;
}

void readHLTProbe()
{
    if (!hasFoundHltProbe)
    {
        return;
    }

    // subtract the last reading:
    HLTtotal = HLTtotal - HLTreadings[HLTindex];

    // get temp of HLT probe
    HLTreadings[HLTindex] = readTemperature(probeHLT);

    // add the reading to the total:
    HLTtotal = HLTtotal + HLTreadings[HLTindex];

    // increment htl index
    HLTindex += 1;

    // restart if we reached max readings
    if (HLTindex >= numReadings)
    {
        HLTgotFirstAverage = true;
        HLTindex = 0;
    }

    // calculate the average:
    HLTaverage = HLTtotal / numReadings;

    if (!DEBUG)
    {
        return;
    }

    // debug stuff here
    Serial.print("HLT: ");
    Serial.print(HLTaverage);
}

void readInternalProbe()
{
    tempInternal = readTemperature(probeInternalTemp);

    if (!DEBUG)
    {
        return;
    }

    // debug stuff here
    Serial.print("Internal: ");
    Serial.print(tempInternal);
}

void setupHotLiquorTankProbe()
{
    hasFoundHltProbe = false;

    // look for temp sensor
    if (!sensors.getAddress(probeHLT, 0))
    {
        // notify that no probe was found
        if (DEBUG)
        {
            Serial.println("no hlt probe found");
        }

        return;
    }

    sensors.setResolution(probeHLT, resolution);
    hasFoundHltProbe = true;

    for (int thisReading = 0; thisReading < numReadings; thisReading++)
        HLTreadings[thisReading] = 0;
}

void setupInternalProbe()
{
    hasFoundInternalProbe = false;

    // 2. Internal Temp
    if (!sensors.getAddress(probeInternalTemp, 0))
    {
        // notify that no probe was found
        if (DEBUG)
        {
            Serial.println("no internal probe found");
        }

        return;
    }

    sensors.setResolution(probeInternalTemp, resolution);
    hasFoundInternalProbe = true;
}

int getStrikeTemp()
{
    return HLTaverage;
}

int getBoilTemp()
{
    // using same probe for now
    return HLTaverage;
}

// -------------------------------------------------------------------------------------------

void setupTempProbes()
{
    // set up temp sensors
    sensors.begin();

    if (DEBUG)
    {
        // locate devices on the bus
        Serial.print("Locating devices...");
        Serial.print("Found ");
        deviceCount = sensors.getDeviceCount();
        Serial.print(deviceCount, DEC);
        Serial.println(" devices.");
        Serial.println("");
    }

    // caclulate time to wait for temp probe to return a value
    tempDelay = 750 / (1 << (12 - resolution));

    // disable conversion wait because we handle it now via millis()
    sensors.setWaitForConversion(false);

    setupHotLiquorTankProbe();
    setupInternalProbe();

    // initial temp request
    sensors.requestTemperatures();
}

void loopTempProbes()
{
    if (!(currentMillis - lastTempRequest >= tempDelay))
    {
        return;
    }

    readHLTProbe();
    readInternalProbe();

    // request temperatures again
    sensors.requestTemperatures();

    // log last time temp was taken
    lastTempRequest = currentMillis;
}