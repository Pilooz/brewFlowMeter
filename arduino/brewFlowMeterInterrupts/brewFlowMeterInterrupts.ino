// PinChangeIntExample, version 1.1 Sun Jan 15 06:24:19 CST 2012
// See the Wiki at http://code.google.com/p/arduino-pinchangeint/wiki for more information.
//-------- define these in your sketch, if applicable ----------------------------------------------------------
// You can reduce the memory footprint of this handler by declaring that there will be no pin change interrupts
// on any one or two of the three ports.  If only a single port remains, the handler will be declared inline
// reducing the size and latency of the handler.
//#define NO_PORTB_PINCHANGES // to indicate that port b will not be used for pin change interrupts
//#define NO_PORTC_PINCHANGES // to indicate that port c will not be used for pin change interrupts
//#define NO_PORTD_PINCHANGES // to indicate that port d will not be used for pin change interrupts
// if there is only one PCInt vector in use the code can be inlined
// reducing latency and code size
// define DISABLE_PCINT_MULTI_SERVICE below to limit the handler to servicing a single interrupt per invocation.
// #define       DISABLE_PCINT_MULTI_SERVICE
//-------- define the above in your sketch, if applicable ------------------------------------------------------
#include <PinChangeInt.h>
#include <LiquidCrystal.h>

#define APP_VERSION "1.0"

// Rotary encoder push action on Pin #3 for interrupt 1
#define ENC_PUSH 3 //Label SW on encoder 
#define ENC_A  7  // Label DT on encoder
#define ENC_B  8  // Label CLK on encoder

// LCD Backlight control, PWM pins
#define LCD_R 5
#define LCD_G 6
#define LCD_B 9

// Solenoid Valve, on 13 to have build-in Led status.
#define VLV 13

#define ENC_STEP  0.1
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
LiquidCrystal lcd(14, 15, 16, 17, 18, 19);

boolean debug_mode = true;

// encoder variables
volatile int encoderPos = 0;
volatile int lastReportedPos = 1;
boolean A_set = false;
boolean B_set = false;
int encoder_button_state = 0;

//    uint8_t latest_interrupted_pin;
//    uint8_t interrupt_count[20]={0}; // 20 possible arduino pins

// LCD variables
int lcd_brightness = 100;

// Valve variables 0 : closed, 1:Opened
int vlv_status = LOW;

float app_target_liters = 0;

// Application variables
int app_status = APP_WAITING;
int app_previous_status = APP_WAITING;
int app_choice = CHOICE_CANCEL;

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

  // background color Orange
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
 *  [Run] [Set] [x]
 **************************************************/
void lcd_options_mode() {
  encoderPos = encoderPos%3;
  // background color Orange
  lcd_setbacklight(255, 50, 0);
  // first line
  lcd.setCursor(0, 0);
  lcd.print("Choice ?        "); 
  // second line
  lcd.setCursor(0, 1);
  switch ((int)encoderPos) {
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
  app_choice = encoderPos;
}


/*************************************************
 * Setup for serial
 **************************************************/
void serial_setup() {
  Serial.begin(9600);
  debug("----- BrewFlowMeter Debug -----");
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
  // Encoder settings
  pinMode(ENC_PUSH, INPUT); 
  digitalWrite(ENC_PUSH, HIGH);
  PCintPort::attachInterrupt(ENC_PUSH, &button_pushed, RISING);  // add more attachInterrupt code as required
  pinMode(ENC_A, INPUT); 
  digitalWrite(ENC_A, HIGH);
  PCintPort::attachInterrupt(ENC_A, &doEncoderA, CHANGE);
  pinMode(ENC_B, INPUT); 
  digitalWrite(ENC_B, HIGH);
  PCintPort::attachInterrupt(ENC_B, &doEncoderB, CHANGE);
  encoderPos = 0;

  // Serial settings
  //  if (debug_mode) {
  serial_setup();
  //  }
}

void debug(String s) {
  Serial.println(s);
}

// --------------------------------------------------------
// Main loop
// --------------------------------------------------------
void loop(){ 
  if (lastReportedPos != encoderPos) {
    switch (encoderPos - lastReportedPos) {
    case -1:
      app_target_liters = encoderPos * ENC_STEP;
      break;
    case 1:
      app_target_liters = encoderPos * ENC_STEP;
      break;
    default:
      break;
    } 
    Serial.print(app_target_liters);
    Serial.println("L");
    lastReportedPos = encoderPos;
  }
}

// --------------------------------------------------------
// Interrupts for Rotary encoder
// --------------------------------------------------------
// Interrupt on A changing state
void doEncoderA(){
  // Test transition
  A_set = digitalRead(ENC_A) == HIGH;
  // and adjust counter + if A leads B
  encoderPos += (A_set != B_set) ? -1 : +1;
  if (encoderPos < 0) {
    encoderPos = 0;
  }
}

// Interrupt on B changing state
void doEncoderB(){
  // Test transition
  B_set = digitalRead(ENC_B) == HIGH;
  // and adjust counter + if B follows A
  encoderPos += (A_set == B_set) ? -1 : +1;
  if (encoderPos < 0) {
    encoderPos = 0;
  }
}

/*************************************************
 * setting encoder event to pushed 
 * (when encoder button is pushed)
 * 
 * This is the Interface Controller
 * This is called on interrupt #1
 **************************************************/
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

void button_pushed() {
  Serial.println("Button pushed !");
  noInterrupts();
  //  encoder_button_state = digitalRead(ENC_PUSH);
  if( (millis() - lastDebounceTime) > debounceDelay){
    //    encoder_button_state = RE_PUSHED;
    int previous_screen = app_get_previous_state();

    // See where we were before this event
    switch (previous_screen) {
    case APP_WAITING:   
      // We were on Waiting state, so go to APP_OPTIONS
      vlv_close();
      app_set_state(APP_OPTIONS);
      lcd_options_mode();
      break;

    case APP_RUNNING:    
      // We were in running mode, so close valve and go to APP_WAITING
      vlv_close();
      app_set_state(APP_WAITING);
//      lcd_waiting_mode();
      break;

    case APP_SETTING:   
      // we were in setting mode, so validate setting and go to APP_WAITING
      vlv_close();
      app_set_state(APP_WAITING);
      break;

    case APP_OPTIONS:   
      // We were in options mode, so see which option was choosen.
      switch (app_choice) {
      case CHOICE_CANCEL:
        app_set_state(APP_WAITING); // Canceling any action, go to waiting state
        break;
      case CHOICE_RUNNING:
        vlv_open();
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
    Serial.print("app_state=");
    Serial.println(app_get_state());
    Serial.print("encoder_button_state=");
    Serial.println(encoder_button_state);
    Serial.print("valve=");
    Serial.println(vlv_status);
  } 
  interrupts();
}




