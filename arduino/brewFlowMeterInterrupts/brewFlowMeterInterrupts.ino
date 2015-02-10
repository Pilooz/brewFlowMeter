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

// Rotary encoder push action on Pin #3 for interrupt 1
#define ENC_PUSH 3 //Label SW on encoder 
#define ENC_A  7  // Label DT on encoder
#define ENC_B  8  // Label CLK on encoder

boolean debug_mode = true;

//volatile unsigned 
volatile int encoderPos = 0;
//unsigned
volatile int lastReportedPos = 1;

boolean A_set = false;
boolean B_set = false;
//    uint8_t latest_interrupted_pin;
//    uint8_t interrupt_count[20]={0}; // 20 possible arduino pins

// --------------------------------------------------------
// Setup
// --------------------------------------------------------
void setup() {
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
  if (debug_mode) {
  Serial.begin(9600);
  debug("----- BrewFlowMeter Debug -----");
  }
}

void debug(String s) {
    Serial.println(s);
}

// --------------------------------------------------------
// Main loop
// --------------------------------------------------------
void loop(){ 
  if (lastReportedPos != encoderPos) {
    Serial.print("Index:");
    Serial.print(encoderPos, DEC);
    Serial.println();
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

// Interrupt when button is pushed
void button_pushed() {
  Serial.print("Button pushed "); 
  Serial.println("!");
  encoderPos = 0;
}


