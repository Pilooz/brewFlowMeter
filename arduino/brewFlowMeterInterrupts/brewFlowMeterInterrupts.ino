/*********************************************************************************
 * BrewFlowMeter v1.0 by Pilooz.
 * 
 * This program uses the PinChangeInt Library to get interruptions on more than two pins
 * with Arduino UNO.
 * See the Wiki at http://code.google.com/p/arduino-pinchangeint/wiki for more information.
 * 
 * Backlight RGB control by https://learn.adafruit.com/character-lcds/rgb-backlit-lcds
 * 
 * @TODO : code optimization
 * -> delete getters and setters for app_(previous_)status 
 * -> See what PORT doesn't need any interrupt (think about PORT D?)
 * code implementation
 * -> See what it blinks when starting program (after setting screen)
 * -> Bug on total liter count : after several flown, eeprom r/w pb ?
 * -> As the debouncing is old style (with pullup pins) https://hifiduino.files.wordpress.com/2010/10/rotenc.jpg
 *    put the encoder pins A&B in pullup mode and test.
 *    see if the debounce delay is not to long ?
 
 **********************************************************************************/
#define NO_PORTC_PINCHANGES // to indicate that port c will not be used for pin change interrupts

#include <EEPROM.h>
#include <PinChangeInt.h>
#include <LiquidCrystal.h>
#include <Wire.h>

#define APP_VERSION "1.0"

// Liquid Flow sensor
#define FLW 2

// Rotary encoder push action
#define ENC_PUSH 3 //Label SW on encoder 
#define ENC_A  7  // Label DT on encoder
#define ENC_B  8  // Label CLK on encoder

// LCD Backlight control, PWM pins
#define LCD_R 5
#define LCD_G 6
#define LCD_B 9

// Solenoid Valve, on 11 (to have PWM management for futher flow controlling ?)
#define VLV 11

#define ENC_STEP  0.1

// Application status
#define APP_ERROR   -1000
// App is displaying splash screen while initializing.
#define APP_SPLASH -1
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
#define CHOICE_CANCEL   0
#define CHOICE_RUNNING  1
#define CHOICE_SETTING  2
#define CHOICE_RESET    3

// Constants for eeprom addresses
#define EEPROM_TOTAL_PULSES_ADDR 0
#define EEPROM_CURRENT_PULSES_ADDR 8
#define EEPROM_TARGET_LITERS_ADDR 16


// Liquid Crystal display on pins A0, A1, A2, A3, A4, A5
LiquidCrystal lcd(14, 15, 16, 17, 18, 19);

boolean debug_mode = true;

// encoder variables
volatile int encoderPos = 0;
volatile int lastReportedPos = 1;
boolean A_set = false;
boolean B_set = false;
// The bstate of the button 0 : rest, 1 pushed
int encoder_button_state = 0;

long lastDebounceTimeA = 0;  // the last time the output pin A was toggled
long debounceDelayA = 10;    // the debounce time
long lastDebounceTimeB = 0;  // the last time the output pin B was toggled
long debounceDelayB = 10;    // the debounce time
long lastDebounceTimeP = 0;  // the last time the output Button was pushed
long debounceDelayP = 100;    // the debounce time; increase if the output flickers

//    uint8_t latest_interrupted_pin;
//    uint8_t interrupt_count[20]={0}; // 20 possible arduino pins

// LCD variables
int lcd_brightness = 100;

// Valve variables 0 : closed, 1:Opened
int vlv_status = LOW;

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

float app_target_liters = 0;

// Application variables
int app_status = APP_WAITING;
int app_previous_status = APP_WAITING;
int app_choice = CHOICE_CANCEL;
String app_error = "";


/*************************************************
 * Writing a float in EEPROM
 **************************************************/
void eeprom_write(int addr, float f) {
  unsigned char *buf = (unsigned char*)(&f);
  for ( int i = 0 ; i < (int)sizeof(f) ; i++ ) {
    EEPROM.write(addr+i, buf[i]);
  }
}

/*************************************************
 * Reading a float from EEPROM
 **************************************************/
float eeprom_read(int addr) {
  float f;
  unsigned char *buf = (unsigned char*)(&f);
  for ( int i = 0 ; i < (int)sizeof(f) ; i++ ) {
    buf[i] = EEPROM.read(addr+i);
  }
  return f;
}

/*************************************************
 * Soft Reset, deleting all stored values
 **************************************************/
void soft_reset() {
  eeprom_write(EEPROM_CURRENT_PULSES_ADDR, 0);
  eeprom_write(EEPROM_TOTAL_PULSES_ADDR, 0);
  eeprom_write(EEPROM_TARGET_LITERS_ADDR, 0);
  asm volatile ("  jmp 0");
}

/*************************************************
 * Closing solenoid Valve
 **************************************************/
void vlv_close() {
  if (vlv_status == HIGH) {
    flw_read(false);
    digitalWrite(VLV, LOW);
    vlv_status = LOW;
  }
}

/*************************************************
 * Opening solenoid Valve
 **************************************************/
void vlv_open() {
  if (vlv_status == LOW) {
    flw_read(true);
    digitalWrite(VLV, HIGH);
    vlv_status = HIGH;
  }
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
 * volume calculations
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
    l -= 6;
    l /= 60.0;
    return l;
  }
  return 0;
}

/*************************************************
 * adusting backlight color on lcd 
 * dealing with pct of volume flown
 **************************************************/
void lcd_adjust_backlight(int pct) {
  // mapping pct between 0 and 255
  int p = (int)(255*pct/100);
  //map(pct, 0, 100, 0, 100);
  lcd_setbacklight(p, 255-p, 0);
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
 * Displaying a splash screen at startup
 **************************************************/
void lcd_splash_screen() {
  // first line
  lcd.setCursor(0, 0);
  lcd.print(" BrewFlowMeter ");  
  // second line
  lcd.setCursor(0, 1);
  lcd.print(" v");
  lcd.print(APP_VERSION);
  lcd.print(" by Pilooz ");

  // background color
  for (int i = 0; i < 255; i++) {
    lcd_setbacklight(i, 0, 255-i);
    delay(5);
  }
  for (int i = 0; i < 255; i++) {
    lcd_setbacklight(255-i, i, 0);
    delay(5);
  }
  for (int i = 0; i < 255; i++) {
    lcd_setbacklight(0, 255-i, i);
    delay(5);
  }
}

/*************************************************
 * Displaying data on lcd 
 * Step : APP_OPTIONS
 * displaying :
 *  Choice ?
 *  [Run] [Set] [x] >
 **************************************************/
void lcd_options_mode() {
  int array_options_size = 4;
  String menus[] = {"   [ cancel ]   ","    [ run ]     ","    [ set ]     ","   [ reset ]    "};
  String texts[] = {"Back to main screen","Open valve","Set quantity","Reset stored values"};
  app_choice = abs(encoderPos + array_options_size * 100) % array_options_size;
  String menu = menus[app_choice];
  String text = texts[app_choice];
  if (menu == "" && text == "") {
    menu = "Choose option by"; 
    text = "turning button  ";
  }
  lcd_print_option_screen(menu, text);
}

void lcd_print_option_screen(String l1, String l2) {
  // background color Orange
  lcd_setbacklight(255, 50, 0);
  lcd.setCursor(0, 0);
  lcd.print(l1); 
  lcd.setCursor(0, 1);
  lcd.print(l2); 
}

/*************************************************
 * Displaying data on lcd 
 * Step : APP_SETTING
 * Turning the rotary enc CW increments by 0.1 L
 * turninc CCW decrements by 0.1 L
 * displaying :
 *   Target volume ?
 *   0.0 L 
 **************************************************/
void lcd_setting_mode() {  
  if (encoderPos < 0) {
    encoderPos = 0;
  }
  app_target_liters = encoderPos * ENC_STEP;
  // background color Orange
  lcd_setbacklight(255, 165, 0);
  // first line
  lcd.setCursor(0, 0);
  lcd.print("Target volume ? "); 
  // second line
  lcd.setCursor(0, 1);
  lcd.print(app_target_liters);
  lcd.print(" L ");
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
  
  if (vlv_status == HIGH) {
    // Set backlight to a various color that say it's open.
    // The color changes on pct increase.
    lcd_adjust_backlight(pct);
  }
  
  // see if we got 100% of target?
  if (pct == 100) {
    // Forcing waiting State, this stat closes valve
    app_set_state(APP_WAITING);
  }
  
  // first line
  lcd.setCursor(0, 0);
  lcd.print(flw_rate, 0);
  lcd.print("Hz "); 
  lcd.print(total_liters);
  lcd.print(" L");
  // second line
  lcd.setCursor(0, 1);
  lcd.print((int)pct);
  lcd.print("% ");
  lcd.print(liters);
  lcd.print("/");
  lcd.print(app_target_liters);
  lcd.print(" L");
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
 * Setup for serial
 **************************************************/
void lcd_message(String msg) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("/!\\ /!\\ /!\\ /!\\ ");
  lcd.setCursor(0, 1);
  lcd.print(msg);
}  

/*************************************************
 * Setup for serial
 **************************************************/
void serial_setup() {
  Serial.begin(9600);
  Serial.println("Flow sensor and rotary encoder test!");
}

// --------------------------------------------------------
// Setup
// --------------------------------------------------------
void setup() {
  // LCD
  // Setup backlight color.
  pinMode(LCD_R, OUTPUT);
  pinMode(LCD_G, OUTPUT);
  pinMode(LCD_B, OUTPUT);
  lcd.begin(16, 2);
  lcd.clear();
  // Slash screen
  lcd_splash_screen();

  // Solenoid Valve
  pinMode(VLV, OUTPUT);
  digitalWrite(VLV, LOW);
  vlv_status = 0;

  // Liquid Flow meter settings
  pinMode(FLW, INPUT_PULLUP);
  //digitalWrite(FLW, LOW);
  flw_last_pinstate = HIGH;
  flw_read(false);
  //PCintPort::attachInterrupt(FLW, &flw_read, RISING);
  
  // Encoder settings
  pinMode(ENC_PUSH, INPUT); 
  digitalWrite(ENC_PUSH, HIGH);
  PCintPort::attachInterrupt(ENC_PUSH, &button_pushed, RISING);
  pinMode(ENC_A, INPUT); 
  digitalWrite(ENC_A, HIGH);
  PCintPort::attachInterrupt(ENC_A, &doEncoderA, CHANGE);
  pinMode(ENC_B, INPUT); 
  digitalWrite(ENC_B, HIGH);
  PCintPort::attachInterrupt(ENC_B, &doEncoderB, CHANGE);
  encoderPos = 0;
  lastReportedPos = 0;
  encoder_button_state = 0;

  // Setting initial state for screen application  
  app_previous_status = APP_SPLASH;
  app_choice = CHOICE_CANCEL;

  // Read saved values from eeprom
  flw_pulses = eeprom_read(EEPROM_CURRENT_PULSES_ADDR);
  flw_total_pulses = eeprom_read(EEPROM_TOTAL_PULSES_ADDR);
  app_target_liters = eeprom_read(EEPROM_TARGET_LITERS_ADDR);
  flw_pulses_old = flw_pulses;  
  
  serial_setup();
}

// --------------------------------------------------------
// Main loop
// --------------------------------------------------------
void loop(){ 
  // If encoder has moved or has been pushed
  if (encoder_button_state == 1 || (lastReportedPos != encoderPos) || (flw_pulses_old != flw_pulses) ) {
    lcd.clear();

    switch (app_get_state()) {
    case APP_WAITING: // App is waiting for sensors or buttons changes : Valve is closed
      vlv_close();
      // Background color is blue, dodgerBlue.
      lcd_setbacklight(30, 144, 255);
      // displaying current passing volume, desired volume, total volume, flowrate
      lcd_waiting_mode();
      // push button may open valve after APP_OPTIONS mode
      break;
    case APP_RUNNING: // App is running water thru valve : valve is opened
      vlv_open();
      // Saving the current flw_pulses
      eeprom_write(EEPROM_CURRENT_PULSES_ADDR, flw_pulses);
      // displaying current passing volume, desired volume, total volume, flowrate
      lcd_running_mode();
      flw_pulses_old = flw_pulses;
      // Add delay to avoid blinking screen. This is not a pb for flowmeter reading
      // as it is read every milliseconde.
      delay(100);
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
      // Close the valve by security
      vlv_close();
      // see if we need to reset something ? treating with errors
      lcd_message(app_error);
      app_set_state(APP_ERROR);
      break;
    }
    lastReportedPos = encoderPos;
    encoder_button_state = 0; 
  }
}

// --------------------------------------------------------
// Interrupts for Rotary encoder
// --------------------------------------------------------
// Interrupt on A changing state
void doEncoderA(){
  noInterrupts();
  if( ((long)millis() - lastDebounceTimeA) > debounceDelayA){
    // Test transition
    A_set = digitalRead(ENC_A) == HIGH;
    // and adjust counter + if A leads B
    encoderPos += (A_set != B_set) ? -1 : +1;
  }
  interrupts();  
}

// Interrupt on B changing state
void doEncoderB(){
  noInterrupts();
  if( ((long)millis() - lastDebounceTimeB) > debounceDelayB){  // Test transition
    B_set = digitalRead(ENC_B) == HIGH;
    // and adjust counter + if B follows A
    encoderPos += (A_set == B_set) ? -1 : +1;
  }
  interrupts();  
}

/*************************************************
 * setting encoder event to pushed 
 * (when encoder button is pushed)
 * 
 * This is the Interface Controller
 * This is called on interrupt
 **************************************************/
void button_pushed() {
  noInterrupts();
  if( ((long)millis() - lastDebounceTimeP) > debounceDelayP){
    // Marking the utton as pushed
    encoder_button_state = 1;
    int previous_screen = app_get_previous_state();
    // See where we were before this event
    switch (previous_screen) {
    case APP_SPLASH:   
      // We were on Waiting state, so go to APP_OPTIONS
      app_set_state(APP_WAITING);
      break;

    case APP_WAITING:   
      // We were on Waiting state, so go to APP_OPTIONS
      app_set_state(APP_OPTIONS);
      break;

    case APP_RUNNING:    
      // We were in running mode, so close valve and go to APP_WAITING
      // here we can write flw_total_pulses to EEPROM
      eeprom_write(EEPROM_TOTAL_PULSES_ADDR, flw_total_pulses);
      app_set_state(APP_WAITING);
      break;

    case APP_SETTING:   
      // we were in setting mode, so validate setting and go to APP_WAITING
      // here we can write app_target_liters to EEPROM
      eeprom_write(EEPROM_TARGET_LITERS_ADDR, app_target_liters);
      // Setting the pulses to zero.
      flw_pulses = 0;
      eeprom_write(EEPROM_CURRENT_PULSES_ADDR, flw_pulses);
      app_set_state(APP_WAITING);
      break;

    case APP_OPTIONS:   
      // We were in options mode, so see which option was choosen.
      switch (app_choice) {
      case CHOICE_CANCEL:
        // init flow meter variables
        flw_pulses_old = flw_pulses;
        app_set_state(APP_WAITING); // Canceling any action, go to waiting state
        break;
      case CHOICE_RUNNING:
        app_set_state(APP_RUNNING); // Go to running mode, valve opened
        break;
      case CHOICE_SETTING:
        app_set_state(APP_SETTING);
        break;
      case CHOICE_RESET:
        soft_reset();
        break;
      default:
        break;
      }
      break;
    default:
      // see if we need to reset something ?
      app_error = "No known state for LCD ?";
      app_set_state(APP_ERROR);
      break;
    } 
  } 
  interrupts();
}

/*************************************************
 * interruptions for flowsensor reading.
 **************************************************/
// Interrupt is called once a millisecond, looks for any flw_pulses from the sensor!
SIGNAL(TIMER0_COMPA_vect) {
  uint8_t x = digitalRead(FLW);
  if (x == flw_last_pinstate) {
    flw_last_ratetimer++;
    flw_pulses_old = flw_pulses;
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

void flw_read(boolean v) {
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








