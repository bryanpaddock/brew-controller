// Copyright (C) 2015  Bryan Paddock

// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <OneWire.h>
#include <DallasTemperature.h>
#include <PID_v1.h>

const boolean DEBUG = false;

/** temperature probes **/

#define ONE_WIRE_BUS 8
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress HLTprobe = { 0x28, 0xFF, 0xBF, 0x3D, 0x64, 0x14, 0x04, 0x30 };
int HLTprobeEnabled = false;
double HLTlongTemp;
int HLTtemp;

// ds18b20 takes time to respond. bypass that with a timer.
unsigned long lastTempRequest;

// set temp probe resolution.
const int resolution = 9;

// the actual delay is calculated in setup() based on resolution
int tempDelay;

//DeviceAddress insideThermometer = { 0x28, 0x06, 0x36, 0x6A, 0x05, 0x00, 0x00, 0x0A };

/** shift registers + 7segment display **/

#define LATCH 6
#define CLK 4
#define DATA 2

const byte digitsHex[]= {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x67};
const byte digitsBits[] = 
{
  B00111111,  // 0
  B00000110,  // 1
  B01011011,  // 2
  B01001111,  // 3
  B01100110,  // 4
  B01101101,  // 5
  B01111101,  // 6
  B00000111,  // 7
  B01111111,  // 8
  B01100111,   // 9
};

/** Hot Liquor Tank PID management **/

boolean pidHLT = true;
const int ledPID = 10;  // led to notify of PID enabled

double Setpoint, Input, Output;
double aggKp=50, aggKi=0.2, aggKd=10;
double consKp=1, consKi=0.05, consKd=0.25;

PID myPID(&Input, &Output, &Setpoint, consKp, consKi, consKd, DIRECT);

int pidWindowSize = 5000;
unsigned long pidWindowStartTime;

/** solid state relay pins **/

const int ssrHLT = 11;
const int ssrBK1 = 12;
//const int ssrBK2 = 13;

boolean ssrHLTstate = false;
boolean ssrBK1state = false;

/** switches **/

// 0 = off
// 1 = hot liquor tank
// 2 = boil kettle single element
// 3 = boil kettle double elements

int switchCurrentElement;

// HLT temp setpoint
int HLTSetPoint = 40;

// boil kettle duty cycle. this will be converted to seconds out of the duty period
int bkDutyCycle;

// duty period is 10 seconds
const int bkDutyPeriod = 10000;

unsigned long currentMillis;
unsigned long bkDutytTimer;

void setup()
{
  // set up the shift registers for the 7segments
  pinMode(LATCH, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(DATA, OUTPUT);

  if (DEBUG)
    Serial.begin(9600);

  // set up temp sensors
  sensors.begin();

  // caclulate time to wait for temp probe to return a value
  tempDelay = 750 / (1 << (12 - resolution));

  // disable conversion wait because we handle it now via millis()
  sensors.setWaitForConversion(false);
  
  // 1. Hot Liquor Tank
  HLTprobeEnabled = false;
  if (!sensors.getAddress(HLTprobe, 0)) 
    Serial.println("Unable to find Hot Liquour Tank temp probe"); 
  else
  {
    sensors.setResolution(HLTprobe, resolution);
    HLTprobeEnabled = true;
  }
  
  // initial temp request
  sensors.requestTemperatures();

  pinMode(ssrHLT, OUTPUT);
  pinMode(ssrBK1, OUTPUT);
//  pinMode(ssrBK2, OUTPUT);
  
  //tell the PID to range between 0 and the full window size
  myPID.SetOutputLimits(0, pidWindowSize);
  
//  myPID.SetControllerDirection(REVERSE);
}

void loop()
{
  // save current millis so everythings synced
  currentMillis = millis();
  
  // check status of switches
  checkSwitches();
 
  // get temps from probes
  getTemps();
  
  // display it on 7seg
  displayTemp();
  
  // check PIDS
  runElements();
  
  // slow down a bit
  delay(100);
}

void runElements()
{
  runHLTPID();
  runBKElements();
}

void runHLTPID()
{
  // do nothing if HLT PID is not enabled
  if (switchCurrentElement != 1) {
    SwitchOffHLTPID();
    return;
  }

  // set input to long temp
  Input = HLTlongTemp;
  Setpoint = HLTSetPoint;
  
  // adapt the tunings based on how far the temp is from setpoint
  double gap = abs(Setpoint-Input);
  if (gap<5)
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

  if (DEBUG)
  {
    Serial.print(Input);
    Serial.print(" / ");
    Serial.print(Setpoint);
    Serial.print(" = ");
    Serial.println(Output);
  }

  /************************************************
   * turn the output pin on/off based on pid output
   ************************************************/

  if(currentMillis - pidWindowStartTime >= pidWindowSize)
  { 
    // time to shift the Relay Window
    pidWindowStartTime += pidWindowSize;
  }

  if (Input <= Setpoint)
  {
    if(Output <= currentMillis - pidWindowStartTime)
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

void runBKElements()
{
  // switch off if OFF or HLT is selected
  if (switchCurrentElement < 2)
  {
    SwitchOffBK();
    return;
  }
  
  // number of elements to fire
  int numberOfElements = switchCurrentElement - 1;
  
  // work out milliseconds kettle is on duty
  long bkOnDutyTime = (long)bkDutyPeriod * (long)bkDutyCycle /  100;
  
  // work out milliseconds kettle is off duty
  long bkOffDutyTime = (long)bkDutyPeriod - bkOnDutyTime;

  if (DEBUG)
  {
    Serial.print(bkOnDutyTime);
    Serial.print(" / ");
    Serial.println(bkOffDutyTime);
  }
  
  // if bkDutyTime has not elapsed yet, keep element on. else turn off
  if ((currentMillis - bkDutytTimer >= bkOnDutyTime) && (ssrBK1state))
  {
    // switch off element
    SwitchOffBK();

    // log when it switched off    
    bkDutytTimer += bkOnDutyTime;
  }
  else if ((currentMillis - bkDutytTimer >= bkOffDutyTime) && (!ssrBK1state) )
  {
    // switch on elements
    SwitchOnBK(numberOfElements);
    
    // log when it switched on
    bkDutytTimer += bkOffDutyTime;
  }
}

/**
function to fetch switch values and store them in memory
**/

void checkSwitches()
{
  // TODO: add code to check a switch
  // fow now, just set current element to HLT (1)
  switchCurrentElement = 1;
  
  // set boil kettle duty cycle to 99% (2char lcd)
  bkDutyCycle = 80;
}

void SwitchOnBK(int elements)
{
  ssrBK1state = true;
  digitalWrite(ssrBK1, HIGH);
  
  //if (elements > 1)
  //digitalWrite(ssrBK2, HIGH);
}

void SwitchOffBK()
{
  ssrBK1state = false;
  digitalWrite(ssrBK1, LOW);
  //digitalWrite(ssrBK2, LOW);
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
//  digitalWrite(ledPID, HIGH);
}

void SwitchOffHLTPID()
{
  myPID.SetMode(MANUAL);
  pidHLT = false;
//  digitalWrite(ledPID, LOW);
  
  // Switch off SSR too, just in case PID didnt.
  SwitchOffHLTElement();
}

void getTemps()
{
  if ( currentMillis - lastTempRequest >= tempDelay )
  {
    // get temp of HLT probe
    HLTlongTemp = sensors.getTempC(HLTprobe);

    // request temperatures again
    sensors.requestTemperatures();
    
    // log last time temp was taken
    lastTempRequest = currentMillis;
  }
}

void displayTemp()
{
  // if boil kettle selected - show duty cycle
  if (switchCurrentElement > 1)
  {
    writeNumber(bkDutyCycle);
    return;
  }
 
  // default to write temp   
  writeNumber((int)HLTlongTemp);
}

void writeNumber(int number)
{
  digitalWrite(LATCH, LOW);

  // start with first digit being 0
  byte digitOne = digitsBits[0];
  byte digitTwo = digitsBits[number];

  // only 2 digit display currently
  if (number > 100)
    number = 99;
  
  if ( number >= 10 )
  {
    // show each digit separately
    int firstIndex = number / 10;
    int secondIndex = number % 10;

    digitOne = digitsBits[firstIndex];
    digitTwo = digitsBits[secondIndex];
  }
   
  shiftOut(DATA, CLK, MSBFIRST, ~digitTwo);
  shiftOut(DATA, CLK, MSBFIRST, ~digitOne);

  digitalWrite(LATCH, HIGH);
}
