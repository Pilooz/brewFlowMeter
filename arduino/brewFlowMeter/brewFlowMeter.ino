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
// Brewpot max volume in liter (use for mapping max volume)
#define MAX_FLOW_VOLUME 50

// Liquid Flow sensor, Pin #2 for interrupt 0
#define FLW 2

// Rotary encoder push action on Pin #3 for interrupt 1
#define REPUSH 3 //Label SW on encoder 
#define REA  7  // Label DT on encoder
#define REB  8  // Label CLK on encoder
// event on encoder push button
#define RE_WAIT 0
#define RE_PUSHED 1
#define ENC_LITERS_STEP 0.10

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
// App is in confirmation mode, valve is closed, 
// displaying a confirmation message on first line and  [run] [set] [x]
// push button may set desired volume and return to APP_RUNNING, APP_SETTING, or APP_WAITING mode
#define APP_OPTIONS 4
// Choices on Options screen
#define CHOICE_RUNNING 1
#define CHOICE_SETTING 2
#define CHOICE_CANCEL  0


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
//volatile int encoder_pos = 0;
//volatile int encoder_oldpos = 0;
int encoder_pos = 0;
int encoder_oldpos = 0;
//volatile boolean encoder_A = 0;
//volatile boolean encoder_B = 0;
int encoder_pinA_last = LOW;
int n = LOW;
int encoder_button_state = 0;
// use to set target volume or to get choice in options mode
//float encoder_absolute_value = 0;
float encoder_step = 1;

// LCD variables
int lcd_brightness = 100;

// Valve variables 0 : closed, 1:Opened
int vlv_status = 0;

// Application variables
int app_status = APP_WAITING;
int app_previous_status = APP_WAITING;
int app_choice = CHOICE_CANCEL;

float app_target_liters = 0;

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
 * Closing solenoid Valve
 **************************************************/
void vlv_close() {
  digitalWrite(VLV, LOW);
  vlv_status = LOW;
}

/*************************************************
 * Opening solenoid Valve
 **************************************************/
void vlv_open() {
  digitalWrite(VLV, HIGH);
  vlv_status = HIGH;
}

/*************************************************
 * Setup for serial
 **************************************************/
void serial_setup() {
  Serial.begin(9600);
  Serial.print("Flow sensor and rotary encoder test!");
}

/*************************************************
 * reading values on Encoder PinA and Pin B
 **************************************************/
void encoder_read() {
  n = digitalRead(REA);
  if ((encoder_pinA_last == LOW) && (n == HIGH)) {
    if (digitalRead(REB) == LOW) {       
      encoder_pos--;
    } 
    else {
      encoder_pos++;
    }
  } 
  encoder_pinA_last = n;

  // Limit to positive values
  if (encoder_pos < 0) {
    encoder_pos = 0;
  }
}

/*************************************************
 * setting encoder event to pushed 
 * (when encoder button is pushed)
 * 
 * This is the Interface Controller
 * This is called on interrupt #1
 **************************************************/
void encoder_pushed(){
  encoder_button_state = RE_PUSHED;
  int previous_screen = app_get_previous_state();
  // See where we were before this event
  switch (previous_screen) {
  case APP_WAITING:   
    // We were on Waiting state, so go to APP_OPTIONS
    app_set_state(APP_OPTIONS);
    break;

  case APP_RUNNING:    
    // We were in running mode, so close valve and go to APP_WAITING
    app_set_state(APP_WAITING);
    break;

  case APP_SETTING:   
    // we were in setting mode, so validate setting and go to APP_WAITING
    app_set_state(APP_WAITING);
    break;

  case APP_OPTIONS:   
    // We were in options mode, so see which option was choosen.
    switch (app_choice) {
    case CHOICE_CANCEL:
      app_set_state(APP_WAITING); // Canceling any action, go to waiting state
      break;
    case CHOICE_RUNNING:
      app_set_state(APP_RUNNING); // Go to running mode, valve opened
      break;
    case CHOICE_SETTING:
      app_set_state(APP_SETTING);
      break;
    default:
      break;
    }
    break;
  default:
    // see if we need to reset something ?
    break;
  } 
}

/*************************************************
 * Displaying data on serial
 **************************************************/
void debug(float liters) {
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
 * Step : APP_WAITING
 * displaying :
 *  flowrate L/s     total volume L
 *  percent flow %   current passed volume / desired volume 
 **************************************************/
void lcd_waiting_mode() {
  float liters = calculateLiters(flw_pulses);
  float total_liters = calculateLiters(flw_total_pulses);
  float pct = 0;
  if (app_target_liters > 0) {
    pct = (int)(100 *liters / app_target_liters);
  }
  if (vlv_status = HIGH) {
    // Set backlight to blue
    lcd_adjust_backlight(pct);
  }
  // first line
  lcd.setCursor(0, 0);
  lcd.print(flw_rate);
  lcd.print("Hz "); 
  lcd.print(total_liters);
  lcd.print(" L");
  // second line
  display_vlv_status();
  /*
  lcd.setCursor(0, 1);
  lcd.print((int)pct);
  lcd.print("% ");
  lcd.print(liters);
  lcd.print("/");
  lcd.print(app_target_liters);
  lcd.print(" L");
  */
}

/*************************************************
 * Displaying data on lcd 
 * Step : APP_RUNNING
 * displaying :
 *  flowrate L/s     total volume L
 *  percent flow %   current passed volume / desired volume 
 * @TODO : rounding values for a pretty display 
 **************************************************/
void lcd_running_mode() {
  // Same screen as waiting mode for the moment
  lcd_waiting_mode();
}

/*************************************************
 * Displaying data on lcd 
 * Step : APP_SETTING
 * displaying :
 *   Target volume ?
 *   0.0 L 
 **************************************************/
void lcd_setting_mode() {
  //app_target_liters = round(encoder_absolute_value, 2);
  app_target_liters = encoder_pos * ENC_LITERS_STEP;
  // background color Orange
  lcd_setbacklight(255, 165, 0);
  // first line
  lcd.setCursor(0, 0);
  lcd.print("Target volume ?"); 
  // second line
  lcd.setCursor(0, 1);
  lcd.print(app_target_liters);
  lcd.print(" L ");
}

/*************************************************
 * Displaying data on lcd 
 * Step : APP_OPTIONS
 * displaying :
 *  Choice ?
 *  [Run] [Set] [x]
 **************************************************/
void lcd_options_mode() {
  encoder_pos = encoder_pos%3;
  // background color Orange
  lcd_setbacklight(255, 50, 0);
  // first line
  lcd.setCursor(0, 0);
  lcd.print("Choice ?"); 
  lcd.print(" p="); 
  lcd.print(encoder_pos);
  // second line
  lcd.setCursor(0, 1);
  switch ((int)encoder_pos) {
  case 0:
    lcd.print(" run   set  [x] "); 
    break;
  case 1:
    lcd.print("[run]  set   x  "); 
    break;
  case 2:
    lcd.print(" run  [set]  x  "); 
    break;
  default:
    lcd.print(" run   set   x  "); 
    break;
  }
  lcd.print(" L ");
  app_choice = encoder_pos;
}

/*************************************************
 * adusting backlight color on lcd 
 * dealing with pct of volume flown
 **************************************************/
void lcd_adjust_backlight(int pct) {
  // mapping pct between 0 and 255
  map(pct, 0, 100, 0, 255);
  lcd_setbacklight(pct, 255-pct, 0);
}

/*************************************************
 * set backlight color on lcd
 **************************************************/
void lcd_setbacklight(uint8_t r, uint8_t g, uint8_t b) {
  // normalize the red LED - its brighter than the rest!
  //r = map(r, 0, 255, 0, 100);
  //g = map(g, 0, 255, 0, 150);
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
  // LCD
  // Setup backlight color.
  pinMode(LCD_R, OUTPUT);
  pinMode(LCD_G, OUTPUT);
  pinMode(LCD_B, OUTPUT);
  lcd.begin(16, 2);
  lcd.clear();
  
  //Serial
  serial_setup();
  // Solenoid Valve
  pinMode(VLV, OUTPUT);
  digitalWrite(VLV, LOW);
  vlv_status = LOW;

  // Liquid Flow meter
  pinMode(FLW, INPUT);
  digitalWrite(FLW, HIGH);
  flw_last_pinstate = HIGH;

  // Rotary encoder
  pinMode(REA, INPUT_PULLUP);
  pinMode(REB, INPUT_PULLUP); 
 //  digitalWrite(REA, HIGH);
  //  digitalWrite(REB, HIGH);
  encoder_button_state = RE_WAIT;

  // Setting initial state for screen application 
  app_set_state(APP_WAITING);
  app_choice = CHOICE_CANCEL;
  // @todo : read eeprom for following value ?
  app_target_liters = 0;

  // setting Interruptions for flow sensor
  flw_interrupt(true);
  // setting interruptions for encoder push action
 ///// attachInterrupt(1, encoder_pushed, RISING);

}

int tmp = 0;

void loop() // run over and over again
{
  encoder_read();

  app_set_state(APP_WAITING);
  lcd_waiting_mode();
  if (encoder_pos != tmp) {
    Serial.print("encoder_pos=");
    Serial.println(encoder_pos);
    tmp = encoder_pos;
  }
  /*  
   // 1. Read all inputs with debounce
   //1.1 reading rotary encoder pins A & B (Push event is on interrupt 1)
   encoder_read();
   // 2. read global application state
   switch (app_get_state()) {
   case APP_WAITING: // App is waiting for sensors or buttons changes : Valve is closed
   vlv_close();
   // Background color is blue, dodgerBlue.
   lcd_setbacklight(30, 144, 255);
   // displaying current passing volume, desired volume, total volume, flowrate
   lcd_waiting_mode();
   // push button may open valve after APP_CONFIRM mode
   break;
   case APP_RUNNING: // App is running water thru valve : valve is opened
   vlv_open();
   // displaying current passing volume, desired volume, total volume, flowrate
   lcd_running_mode();
   // push button may interrupt to close valve and return to APP_WAITING mode
   break;
   case APP_SETTING: // App is in setting mode, valve is closed
   vlv_close();
   // displying 'setting mode' on first line and desired volume to adjust on second line
   lcd_setting_mode();
   // turing the rotary encoder adjust desired volume of water,
   // push button may set adjusted volume of water and return to APP_WAITING mode
   break;
   case APP_OPTIONS: // App is in confirmation mode, valve is closed
   vlv_close();
   // displaying a confirmation message on first line and  run / set / cancel choice on second line.
   lcd_options_mode();
   // push button may set desired volume and return to APP_WAITING mode
   break;
   default:
   // see if we need to reset something ?
   break;
   } 
   delay(100);
   */
}

void display_vlv_status() {
    lcd.setCursor(0, 1);
    lcd.print("v=");
    lcd.print(vlv_status);
    delay(2000);
}






