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
include "LiquidCrystal.h"

// Liquid Flow sensor
#define FLW 2
// Rotary encoder
#define REA  A0
#define REB  A1
#define REPUSH A2 
// 
// Liquid Crystal display
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

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

void useInterrupt(boolean v) {
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
 * Setup
 **************************************************/
void setup() {
  //Serial
  Serial.begin(9600);
  Serial.print("Flow sensor and rotary encoder test!");

  // LCD
  //lcd.begin(16, 2);

  // Liquid Flow meter
  pinMode(FLW, INPUT);
  digitalWrite(FLW, HIGH);
  flw_last_pinstate = digitalRead(FLW);

  // Rotary encoder
  pinMode(REA, INPUT_PULLUP);
  pinMode(REB, INPUT_PULLUP); 
  encoder_A = (boolean)digitalRead(REA); //initial value of channel A;
  encoder_B = (boolean)digitalRead(REB); //and channel B

  // Interuptions
  // Keep interruption for ButtonPress, not for rotary encoder
  // rotary encoder A channel on interrupt 1 (arduino's pin 3)
  //attachInterrupt(1, doEncoderA, RISING);
  // rotary encoder B channel pin on interrupt 2 (arduino's pin 4)
  //attachInterrupt(21, doEncoderB, CHANGE); 
  useInterrupt(true);
}

void toSerial(float liters) {
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

void toLCD(float liter) {
  if (flw_pulses_old != flw_pulses) {
    //lcd.setCursor(0, 0);
    //lcd.print("flw_pulses:"); lcd.print(flw_pulses, DEC);
    //lcd.print(" Hz:");
    //lcd.print(flw_rate);
    //lcd.print(flw_rate);
    //lcd.setCursor(0, 1);
    //lcd.print(liters); lcd.print(" Liters ");
  }
}

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

void loop() // run over and over again
{
  float liters = calculateLiters(flw_pulses);

  //toLCD(liters);
  toSerial(liters);
  
  delay(100);
}

/*************************************************
 * Interruptions
 **************************************************/
void doEncoderA(){
  encoder_B ? encoder_pos--:  encoder_pos++;
}

void doEncoderB(){
  encoder_B = !encoder_B; 
}


