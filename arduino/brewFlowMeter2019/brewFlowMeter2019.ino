/*********************************************************************************
   BrewFlowMeter v2.0 by Pilooz - 2019
 **********************************************************************************/
#include <Wire.h>
#include "config.h"
#include "rgb_lcd.h"
#include "valve.h"
#include "encoder.h"
#include "flowmeter.h"
Valve valve(VLV);
#include "screens.h"
#include "application.h"

/*************************************************
   Setup
 **************************************************/
void setup()
{
  Serial.begin (115200);
  
  // Setup LCD
  lcd_setup();

  // Setup encoder
  encoder_setup();

  // setup flowmeter
  flowmeter_setup();

  // Setup applcation
  application_setup();
  
}
void loop() {
  handle_application_screens();
  handle_application_choices();
  
}

  /*********************************************************************************************************
  END FILE
*********************************************************************************************************/
