#include <PinChangeInt.h>
#include <EEPROM.h>

// Constants for eeprom addresses
#define EEPROM_TOTAL_PULSES_ADDR 0
#define EEPROM_CURRENT_PULSES_ADDR 8
#define EEPROM_TARGET_LITERS_ADDR 16

// Liquid Flow meter variables
// count how many flw_pulses!
volatile uint16_t flw_pulses = 0;
// debounce display
volatile uint16_t flw_pulses_old = 0;
// Total flw_pulses
uint16_t flw_total_pulses = 0;
// track the state of the pulse pin
volatile uint8_t flw_last_pinstate;
// you can try to keep time of how long it is between flw_pulses
volatile uint32_t flw_last_ratetimer = 0;
// and use that to calculate a flow rate
volatile float flw_rate;

// Expose these variables to application
// Target of number of liter to deliver
float app_target_liters = 0;
float app_pct_target_liters = 0; // % of target reached
float flowmeter_liters, flowmeter_total_liters;
boolean flowmeter_was_turning = false;

/*************************************************
   Writing a float in EEPROM
 **************************************************/
void eeprom_write(int addr, float f) {
  unsigned char *buf = (unsigned char*)(&f);
  for ( int i = 0 ; i < (int)sizeof(f) ; i++ ) {
    EEPROM.write(addr + i, buf[i]);
  }
}

/*************************************************
   Reading a float from EEPROM
 **************************************************/
float eeprom_read(int addr) {
  float f;
  unsigned char *buf = (unsigned char*)(&f);
  for ( int i = 0 ; i < (int)sizeof(f) ; i++ ) {
    buf[i] = EEPROM.read(addr + i);
  }
  return f;
}

/*************************************************
   Deleting all stored values
 **************************************************/
void flowmeter_reset() {
  eeprom_write(EEPROM_CURRENT_PULSES_ADDR, 0);
  eeprom_write(EEPROM_TOTAL_PULSES_ADDR, 0);
  eeprom_write(EEPROM_TARGET_LITERS_ADDR, 0);
}

/*************************************************
   Calculate target liters in function
   of a predefined step
 **************************************************/
void flowmeter_calculate_target_liters() {
  app_target_liters += encoderPosCount * ENC_STEP;
  if ( app_target_liters < 0 ) {
    app_target_liters = 0;
  }
}

/*************************************************
   Calculate the pct of target liters we have reached
 **************************************************/
void flowmeter_calculate_pct_of_target_liters() {
  app_pct_target_liters = 0;
  if (app_target_liters > 0) {
    app_pct_target_liters = (int)(100 * flowmeter_liters / app_target_liters);
  }
}

/*************************************************
  volume calculations
**************************************************/
float calculateLiters(uint16_t p) {
  // Sensor Frequency (Hz) = 7.5 * Q (Liters/min)
  // Liters = Q * time elapsed (seconds) / 60 (seconds/minute)
  // Liters = (Frequency (flw_pulses/second) / 7.5) * time elapsed (seconds) / 60
  // Liters = flw_pulses / (7.5 * 60)
  // if a brass sensor use the following calculation
  if (p > 0) {
    float l = p;
    l /= 8.1;
    //??????? Need to be precised ????    l -= 6;
    l /= 60.0;
    return l;
  }
  return 0;
}

/*************************************************
   interruptions for flowsensor reading.
 **************************************************/
void flowmeter_read() {
  uint8_t x = digitalRead(FLW);
  if (x == flw_last_pinstate) {
    flw_last_ratetimer++;
    flw_pulses_old = flw_pulses;
    return; // nothing changed!
  }
  if (x == HIGH) {
    //low to high transition!
    flw_total_pulses++;
    flw_pulses++;
    flowmeter_was_turning = true;
    flowmeter_liters = calculateLiters(flw_pulses);
    flowmeter_total_liters = calculateLiters(flw_total_pulses);
  }
  flw_last_pinstate = x;
  flw_rate = 1000.0;
  flw_rate /= flw_last_ratetimer; // in hertz
  flw_last_ratetimer = 0;
}

/*************************************************
   Setup flowmeter (to be included in general setup)
 **************************************************/
void flowmeter_setup() {
  // Liquid Flow meter settings
  pinMode(FLW, INPUT_PULLUP);
  attachPinChangeInterrupt(FLW, flowmeter_read, CHANGE);
  flw_last_pinstate = HIGH;
  flowmeter_read();

  // Read saved values from eeprom
  flw_pulses = eeprom_read(EEPROM_CURRENT_PULSES_ADDR);
  flw_total_pulses = eeprom_read(EEPROM_TOTAL_PULSES_ADDR);
  app_target_liters = eeprom_read(EEPROM_TARGET_LITERS_ADDR);
  
  // Init variables
  flw_pulses_old = flw_pulses;
  flowmeter_liters = calculateLiters(flw_pulses);
  flowmeter_total_liters = calculateLiters(flw_total_pulses);
}

/*************************************************
   Testing flowmeter calculations
 **************************************************/
void flowmeter_test() {
  Serial.println(flowmeter_liters);
  Serial.println(flowmeter_total_liters);
  delay(500);
}
