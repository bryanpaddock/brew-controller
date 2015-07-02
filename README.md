# brew-controller
Home Brewery Control Panel

The aim of this project is to create a open source control panel that
can be made much cheaper than your average ElectricBrewery.com control
panel.

I'm simply uploading it here so that anyone else can use it if they please
or learn from it (or better it!)

The current set up I have is:
 - Arduino
 - DS18B20 Temperature probes
 - Fotek 25A SSR's
 - 12V power supply for SSRs (not necessary. arduino can power the SSR's itself)
 - 7segment LCD via shift register

The current objective of the control panel is to:
 - Heat and maintain temp of strike/sparge water.
 - Manage element power during boil.

So far its functionality includes:
 - Reads DS18B20 Temp sensors. (bypasses conversion time by waiting in background)
 - Temps are checked as soon as they are ready. Currently 9bit which means every 90ms.
 - 74HC595 shift registers to display temperature on 2 digit 7 segment display
 - PID mode for HLT to reach + maintain a temperature.
 - Aggressive/Conservative PID settings to avoid overshooting temp ranges
 - Power mode for Boil Kettle so you can set a power percentage.
 - Single or Dual element power in Boil Kettle
 - Non blocking code (no delay()) so everything feels smooth

It has settings the brewer can set to work with their brewery:
 - The PID and Power Mode Window time can be adjusted so element can be on for longer
 - Duty Cycle can be changed in real-time. (100% power, 80%, 75% etc). Cycle is converted to time it stays
   on for during window time.
 - Currently runs in deg Celsius but can be changed to F with small changes.
 - Can change element select in real time (Off/ HLT / Boil Single / Boil Dual)