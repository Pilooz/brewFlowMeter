# brewFlowMeter
An Arduino based flowmeter for home brewing
 
 *_Work in Progress, nothing already done yet..._*
 
## Features
### Solenoid Valve : 
It's controled by the Arduino. By default, it is closed.

### Rotary encoder :
  The rotary encoder is used to set the desired quantity of water to be flown.
  A push on Rotary encoder sets the desired quantity
  A second push open/close solenoid valve alternatively.
  
### The liquidCrystal displayer :
It displays :
 - Tthe total amont of water flown since the system has been reset.
 - The number of liters per minute,
 - The desired quantity of water set 
 - the total of water flown since the solenoid valve has opened.

Its background color is sofware controled :
  - blue when the solenoid valve is closed.
  - Green when solenoid valve is open
    -> goes to *orange* and *red* as water volume flowed reaches the desired volume of water.
  - pink when setting desired quantity ?
