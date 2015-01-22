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

// Liquid Flow sensor
#define FLOWSENSORPIN 2
// Rotary encoder
#define encoder0PinA  3
#define encoder0PinB  4
// Liquid Crystal display
//LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Liquid Flow meter variables
// count how many pulses!
volatile uint16_t pulses = 0;
// debounce display
volatile uint16_t pulsesOld = 0;
// Total pulses
uint16_t totalPulses = 0;
// track the state of the pulse pin
volatile uint8_t lastflowpinstate;
// you can try to keep time of how long it is between pulses
volatile uint32_t lastflowratetimer = 0;
// and use that to calculate a flow rate
volatile float flowrate;

// Rotary encoder variables
volatile int encoder0Pos = 0;
volatile int encoder0PosOld = 0;
volatile boolean PastA = 0;
volatile boolean PastB = 0;


// Interrupt is called once a millisecond, looks for any pulses from the sensor!
SIGNAL(TIMER0_COMPA_vect) {
  uint8_t x = digitalRead(FLOWSENSORPIN);
  if (x == lastflowpinstate) {
    lastflowratetimer++;
    return; // nothing changed!
  }
  if (x == HIGH) {
    //low to high transition!
    pulses++;
  }
  lastflowpinstate = x;
  flowrate = 1000.0;
  flowrate /= lastflowratetimer; // in hertz
  lastflowratetimer = 0;
  totalPulses = pulses;
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
  pinMode(FLOWSENSORPIN, INPUT);
  digitalWrite(FLOWSENSORPIN, HIGH);
  lastflowpinstate = digitalRead(FLOWSENSORPIN);

  // Rotary encoder
  pinMode(encoder0PinA, INPUT_PULLUP);
  pinMode(encoder0PinB, INPUT_PULLUP); 
  PastA = (boolean)digitalRead(encoder0PinA); //initial value of channel A;
  PastB = (boolean)digitalRead(encoder0PinB); //and channel B

  // Interuptions
  // Keep interruption for ButtonPress, not for rotary encoder
  // rotary encoder A channel on interrupt 1 (arduino's pin 3)
  //attachInterrupt(1, doEncoderA, RISING);
  // rotary encoder B channel pin on interrupt 2 (arduino's pin 4)
  //attachInterrupt(21, doEncoderB, CHANGE); 
  useInterrupt(true);
}

void toSerial(float liters) {
  if (pulsesOld != pulses || encoder0PosOld != encoder0Pos) {
    Serial.print("Freq: "); 
    Serial.println(flowrate);
    Serial.print("Pulses: "); 
    Serial.println(pulses, DEC);
    Serial.print(liters); 
    Serial.println(" Liters");

    Serial.print("Total Pulses: "); 
    Serial.println(totalPulses, DEC);
    Serial.print("Total Liters: "); 
    Serial.print(calculateLiters(totalPulses)); 
    Serial.println(" L");
     Serial.print("Encoder value: "); 
    Serial.println(encoder0Pos);
   
    pulsesOld = pulses;
    encoder0PosOld = encoder0Pos;
  }
}

void toLCD(float liter) {
  if (pulsesOld != pulses) {
    //lcd.setCursor(0, 0);
    //lcd.print("Pulses:"); lcd.print(pulses, DEC);
    //lcd.print(" Hz:");
    //lcd.print(flowrate);
    //lcd.print(flowrate);
    //lcd.setCursor(0, 1);
    //lcd.print(liters); lcd.print(" Liters ");
  }
}

float calculateLiters(uint16_t p) {
  // Sensor Frequency (Hz) = 7.5 * Q (Liters/min)
  // Liters = Q * time elapsed (seconds) / 60 (seconds/minute)
  // Liters = (Frequency (Pulses/second) / 7.5) * time elapsed (seconds) / 60
  // Liters = Pulses / (7.5 * 60)
  // if a brass sensor use the following calculation
  float l = p;
  l /= 8.1;
  l -= 6;
  l /= 60.0;
  return l;
}

void loop() // run over and over again
{
  float liters = calculateLiters(pulses);

  //toLCD(liters);
  toSerial(liters);
  
  delay(100);
}

/*************************************************
 * Interruptions
 **************************************************/
void doEncoderA(){
  PastB ? encoder0Pos--:  encoder0Pos++;
}

void doEncoderB(){
  PastB = !PastB; 
}


