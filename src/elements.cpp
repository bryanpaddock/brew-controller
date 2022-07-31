#include <Arduino.h>
#include <PID_v1.h>
#include "elements.h"
#include "probes.h"
#include "main.h"

// pins
const int ssrHLT = 13; // d5
const int ssrBK = 12;  // d4

bool pidHLT = false;

double Setpoint, Input, Output;
double aggKp = 50, aggKi = 0.2, aggKd = 10;
double consKp = 1, consKi = 0.05, consKd = 0.25;

PID myPID(&Input, &Output, &Setpoint, consKp, consKi, consKd, DIRECT);

unsigned long pidWindowSize = 5000;
unsigned long pidWindowStartTime;

// boil kettle duty cycle. this will be converted to seconds out of the duty period
unsigned long bkDutyCycle;

// duty period is 10 seconds
unsigned long bkDutyPeriod = 10000;
unsigned long bkDutytTimer;

bool ssrHLTstate = false;
bool ssrBKstate = false;

bool isHeatingStrike = false;
bool isBoiling = false;

void setupElements()
{
    pinMode(ssrHLT, OUTPUT);
    pinMode(ssrBK, OUTPUT);

    // set boil kettle duty cycle to 99%
    bkDutyCycle = 80;

    // tell the PID to range between 0 and the full window size
    myPID.SetOutputLimits(0, pidWindowSize);

    // myPID.SetControllerDirection(REVERSE);
}

// ------------------------------------------------------------------------------------------------

void SwitchOnBK()
{
    ssrBKstate = true;
    digitalWrite(ssrBK, HIGH);
}

void SwitchOffBK()
{
    ssrBKstate = false;
    digitalWrite(ssrBK, LOW);
}

void SwitchOnHLTElement()
{
    digitalWrite(ssrHLT, HIGH);
}

void SwitchOffHLTElement()
{
    digitalWrite(ssrHLT, LOW);
}

void SwitchOnHLTPID()
{
    myPID.SetMode(AUTOMATIC);
    pidHLT = true;
}

void SwitchOffHLTPID()
{
    myPID.SetMode(MANUAL);
    pidHLT = false;

    // Switch off SSR too, just in case PID didnt.
    SwitchOffHLTElement();
}

void runHLTPID()
{
    if (!isHeatingStrike)
    {
        // if we are not heating the strike, then we need to turn off the PID
        SwitchOffHLTPID();
        return;
    }

    // set input to long temp
    Input = HLTaverage;
    Setpoint = HLTSetPoint;

    // adapt the tunings based on how far the temp is from setpoint
    double gap = abs(Setpoint - Input);
    if (gap < 5)
    {
        // we're close to setpoint, use conservative tuning parameters
        myPID.SetTunings(consKp, consKi, consKd);
    }
    else
    {
        // we're far from setpoint, use aggressive tuning parameters
        myPID.SetTunings(aggKp, aggKi, aggKd);
    }

    // if reach the temp, then switch off
    if (Input >= Setpoint)
    {
        // we have reached set point
        // switch off pid and return

        SwitchOffHLTPID();
        return;
    }

    myPID.Compute();

    /************************************************
     * turn the output pin on/off based on pid output
     ************************************************/

    if (currentMillis - pidWindowStartTime >= pidWindowSize)
    {
        // time to shift the Relay Window
        pidWindowStartTime += pidWindowSize;
    }

    if (Input <= Setpoint)
    {
        if (Output <= currentMillis - pidWindowStartTime)
        {
            SwitchOnHLTElement();
        }
        else
        {
            SwitchOffHLTElement();
        }
    }
    else
    {
        SwitchOffHLTElement();
    }
}

// -------------------------------------------------------------------------------------------

void runBKElements()
{
    if (!isBoiling)
    {
        // we're not boiling, so switch off the SSR
        SwitchOffBK();
        return;
    }

    // now we basically take the duty cycle and convert it to a time period

    // work out milliseconds kettle is on duty
    unsigned long bkOnDutyTime = (long)bkDutyPeriod * (long)bkDutyCycle / 100;

    // work out milliseconds kettle is off duty
    unsigned long bkOffDutyTime = (long)bkDutyPeriod - bkOnDutyTime;

    // if bkDutyTime has not elapsed yet, keep element on. else turn off
    if ((currentMillis - bkDutytTimer >= bkOnDutyTime) && (ssrBKstate))
    {
        // switch off element
        SwitchOffBK();

        // log when it switched off
        bkDutytTimer += bkOnDutyTime;
    }
    else if ((currentMillis - bkDutytTimer >= bkOffDutyTime) && (!ssrBKstate))
    {
        // switch on elements
        SwitchOnBK();

        // log when it switched on
        bkDutytTimer += bkOffDutyTime;
    }
}

// ---- strike

void enableStrike()
{
    isHeatingStrike = true;
}

void disableStrike()
{
    isHeatingStrike = false;
}

void setStrikeSetpoint(double setpoint)
{
    if (setpoint < 0)
    {
        setpoint = 0;
    }
    else if (setpoint > 100)
    {
        setpoint = 100;
    }

    HLTSetPoint = setpoint;
}

double getStrikeSetpoint()
{
    return HLTSetPoint;
}

// ----- boil

void enableBoil()
{
    isBoiling = true;
}

void disableBoil()
{
    isBoiling = false;
}

void setBoilDutyCycle(unsigned long dutyCycle)
{
    if (dutyCycle < 0)
    {
        dutyCycle = 0;
    }
    else if (dutyCycle > 100)
    {
        dutyCycle = 100;
    }

    bkDutyCycle = dutyCycle;
}

unsigned long getBoilDutyCycle()
{
    return bkDutyCycle;
}

// ------------------------------------------------------------------------------------------------

void loopElements()
{
    runHLTPID();
    runBKElements();
}