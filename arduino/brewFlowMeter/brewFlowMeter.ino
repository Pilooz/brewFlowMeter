/**********************************************************
 * Liquid flow meters :
 * Connect the red wire to +5V,
 * the black wire to common ground
 * and the yellow sensor wire to pin #2
 * Interruption is 0 on pin #2
 *
 * Rotary encoder :
 * Pin A on Arduino's pin #3
 * Pin B on Arduino's pin #4
 *
 * Liquid flow meters has been written by Limor Fried/Ladyada 
 * for Adafruit Industries.
 * BSD license, check license.txt for more information
 * All text above must be included in any redistribution
 **********************************************************/
//include "LiquidCrystal.h"
#include <LiquidCrystal.h>
#include <Wire.h>

// Liquid Flow sensor, Pin #2 for interrupt 0
#define FLW 2

// Rotary encoder push action on Pin #3 for interrupt 1
#define REPUSH 3 
#define REA  7
#define REB  8

// LCD Backlight control, PWM pins
#define LCD_R 5
#define LCD_G 6
#define LCD_B 9

// Solenoid Valve, on 13 to have build-in Led status.
#define VLV 13

// Application status
// App is waiting for sensors or buttons changes : Valve is closed
// displaying current passing volume, desired volume, total volume, flowrate
// push button may open valve after APP_CONFIRM mode
#define APP_WAITING 0
// App is running water thru valve : valve is opened
// displaying current passing volume, desired volume, total volume, flowrate
// push button may interrupt to close valve and return to APP_WAITING mode
#define APP_RUNNING 1
// App is in setting mode, valve is closed
// displying 'setting mode' on first line and desired volume to adjust on second line
// turing the rotary encoder adjust disered volume of water,
// push button may set adjusted volume of water and return to APP_WAITING mode
#define APP_SETTING 2
// App is in confirmation mode, valve is closed
// displaying a confirmation message on first line and  yes | no choice on second line.
// push button may set desired volume and return to APP_WAITING mode
#define APP_CONFIRM 3

// Liquid Crystal display on pins A0, A1, A2, A3, A4, A5
// to keep pwm and interrupts pins forothers deveices
LiquidCrystal lcd(14, 15, 16, 17, 18, 19);
//LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

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

// Rotary encoder variables
volatile int encoder_pos = 0;
volatile int encoder_oldpos = 0;
volatile boolean encoder_A = 0;
volatile boolean encoder_B = 0;

// LCD variables
int lcd_brightness = 100;

// Valve variables
int vlv_status = 0;

// Application variables
int app_status = APP_WAITING;
int app_previous_status = APP_WAITING;

/*************************************************
 * interruptions for flowsensor reading.
 **************************************************/
// Interrupt is called once a millisecond, looks for any flw_pulses from the sensor!
SIGNAL(TIMER0_COMPA_vect) {
  uint8_t x = digitalRead(FLW);
  if (x == flw_last_pinstate) {
    flw_last_ratetimer++;
    return; // nothing changed!
  }
  if (x == HIGH) {
    //low to high transition!
    flw_pulses++;
  }
  flw_last_pinstate = x;
  flw_rate = 1000.0;
  flw_rate /= flw_last_ratetimer; // in hertz
  flw_last_ratetimer = 0;
  flw_total_pulses = flw_pulses;
}

void flw_interrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
  } 
  else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
  }
}

/*************************************************
 * Setup for lcd
 **************************************************/
void lcd_setup() {
  // Setupbacklight color.
  pinMode(LCD_R, OUTPUT);
  pinMode(LCD_G, OUTPUT);
  pinMode(LCD_B, OUTPUT);
  lcd.begin(16, 2);
  lcd.clear();
}

/*************************************************
 * Setup for solenoid Valve
 * By default it is closed.
 **************************************************/
void vlv_setup() {
  pinMode(VLV, OUTPUT);
  digitalWrite(VLV, LOW);
  vlv_status = digitalRead(VLV);
}

/*************************************************
 * Setup for flowsensor
 **************************************************/
void flw_setup() {
  pinMode(FLW, INPUT);
  digitalWrite(FLW, HIGH);
  flw_last_pinstate = digitalRead(FLW);
}

/*************************************************
 * Setup for serial
 **************************************************/
void serial_setup() {
  Serial.begin(9600);
  Serial.print("Flow sensor and rotary encoder test!");
}

/*************************************************
 * Setup for rotary encoder
 **************************************************/
void encoder_setup() {
  pinMode(REA, INPUT_PULLUP);
  pinMode(REB, INPUT_PULLUP); 
  encoder_A = (boolean)digitalRead(REA); //initial value of channel A;
  encoder_B = (boolean)digitalRead(REB); //and channel B
}

/*************************************************
 * Setup initial state for application
 **************************************************/
void app_setup() {
  app_set_state(APP_WAITING);
}

/*************************************************
 * Displaying data on serial
 **************************************************/
void serial_display(float liters) {
  if (flw_pulses_old != flw_pulses || encoder_oldpos != encoder_pos) {
    Serial.print("Freq: "); 
    Serial.println(flw_rate);
    Serial.print("flw_pulses: "); 
    Serial.println(flw_pulses, DEC);
    Serial.print(liters); 
    Serial.println(" Liters");

    Serial.print("Total flw_pulses: "); 
    Serial.println(flw_total_pulses, DEC);
    Serial.print("Total Liters: "); 
    Serial.print(calculateLiters(flw_total_pulses)); 
    Serial.println(" L");
    Serial.print("Encoder value: "); 
    Serial.println(encoder_pos);

    flw_pulses_old = flw_pulses;
    encoder_oldpos = encoder_pos;
  }
}

/*************************************************
 * Displaying data on lcd
 **************************************************/
void lcd_display(float liters) {
  if (flw_pulses_old != flw_pulses) {
    lcd.setCursor(0, 0);
    lcd.print("flw_pulses:"); 
    lcd.print(flw_pulses, DEC);
    lcd.print(" Hz:");
    lcd.print(flw_rate);
    lcd.print(flw_rate);
    lcd.setCursor(0, 1);
    lcd.print(liters); 
    lcd.print(" Liters ");
  }
}

/*************************************************
 * set backlight color on lcd
 **************************************************/
void lcd_setbacklight(uint8_t r, uint8_t g, uint8_t b) {
  // normalize the red LED - its brighter than the rest!
  r = map(r, 0, 255, 0, 100);
  g = map(g, 0, 255, 0, 150);
  r = map(r, 0, 255, 0, lcd_brightness);
  g = map(g, 0, 255, 0, lcd_brightness);
  b = map(b, 0, 255, 0, lcd_brightness);
  // common anode so invert!
  r = map(r, 0, 255, 255, 0);
  g = map(g, 0, 255, 255, 0);
  b = map(b, 0, 255, 255, 0);
  analogWrite(LCD_R, r);
  analogWrite(LCD_G, g);
  analogWrite(LCD_B, b);
}

/*************************************************
 * volume calculations
 **************************************************/
float calculateLiters(uint16_t p) {
  // Sensor Frequency (Hz) = 7.5 * Q (Liters/min)
  // Liters = Q * time elapsed (seconds) / 60 (seconds/minute)
  // Liters = (Frequency (flw_pulses/second) / 7.5) * time elapsed (seconds) / 60
  // Liters = flw_pulses / (7.5 * 60)
  // if a brass sensor use the following calculation
  float l = p;
  l /= 8.1;
  l -= 6;
  l /= 60.0;
  return l;
}

/*************************************************
 * Setter for application status
 **************************************************/
void app_set_state(int s) {
  app_previous_status = app_get_state();
  app_status = s;
}

/*************************************************
 * Getter for application status
 **************************************************/
int app_get_state() {
  return app_status;
}

/*************************************************
 * Getter for application previous status
 **************************************************/
int app_get_previous_state() {
  return app_previous_status;
}

/*************************************************
 * Setup
 **************************************************/
void setup() {
  //Serial
  //serial_setup();

  // Solenoid Valve
  vlv_setup();

  // LCD
  lcd_setup();

  // Liquid Flow meter
  flw_setup();

  // Rotary encoder
  encoder_setup();

  // Setting initial state for screen application 
  app_setup();
  
  // setting Interuptions for flow sensor
  flw_interrupt(true);
}

void loop() // run over and over again
{
  // 1. Read all inputs with debounce
  // 1.1 read rotaty encoder
  // 2. read global application state
  
  float liters = calculateLiters(flw_pulses);
  lcd_display(liters);
  //serial_display(liters);
  delay(100);
}

/*************************************************
 * Interruptions
 **************************************************
 * void doEncoderA(){
 * encoder_B ? encoder_pos--:  encoder_pos++;
 * }
 * 
 * void doEncoderB(){
 * encoder_B = !encoder_B; 
 * }
 */



