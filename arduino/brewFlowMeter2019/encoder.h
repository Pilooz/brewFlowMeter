#include <PinChangeInt.h>

volatile int encoderPosCount;
volatile int enc_clk_last;
volatile int enc_clk_val, enc_dt_val;
volatile long previousMillisEncoder;
volatile boolean testing_mode = false;

// For application controle : a push changes screen 
// This variable becomes true when button is pushed
// it goes back to false when application controller
// has hanled to it.
boolean button_was_pushed = false;
boolean button_was_turned = false;

/*************************************************
   setter/getter encoder counter
 **************************************************/
void init_encoder_position(int pos = 0) {
  encoderPosCount = pos;
  button_was_turned = false;
}

int get_encoder_position() {
  return encoderPosCount;
}

/*************************************************
   Read the value off encoder.
   sets a relative value :
    +1 : for Clock Wise rotation
    -1 : for Counter Clock Wise rotation
 **************************************************/
void encoder_read() {
  init_encoder_position(0);
  enc_clk_val = digitalRead(ENC_CLK);
  long currentMillis = millis();
  if (enc_clk_val != enc_clk_last && currentMillis - previousMillisEncoder > 50) { // Means the knob is rotating
    previousMillisEncoder = currentMillis;
    button_was_turned = true;
    // if the knob is rotating, we need to determine direction
    // We do that by reading ENC_DT.
    if (digitalRead(ENC_DT) != enc_clk_val) {  // Means pin A Changed first - We're Rotating Clockwise
      encoderPosCount = 1;
    } else { // Otherwise B changed first and we're moving CCW
      encoderPosCount = -1;
    }
    // ---- Values sent to serial only in testing_mode ----
    if (testing_mode) {
      Serial.print ("Rotated: ");
      if (encoderPosCount > 0) {
        Serial.println ("clockwise");
      } else {
        Serial.println("counterclockwise");
      }
      Serial.print("Encoder Position: ");
      Serial.println(encoderPosCount);
    }
    // -----------------------------------------------------
  }
  enc_clk_last = enc_clk_val;
}

/*************************************************
   Returns true if encoder's button has been pushed
 **************************************************/
void encoder_button_pushed() {
  button_was_pushed = false;
  delay(300); //debounce
  // Button detection
  if (digitalRead(ENC_SW) == LOW) {
    button_was_pushed = true;
    if (testing_mode)  {
      Serial.println("Encoder Button was pushed !");
    }
  }
}

/*************************************************
   Setup encoder (to be included in general setup)
 **************************************************/
void encoder_setup() {
  pinMode (ENC_CLK, INPUT);
  pinMode (ENC_DT, INPUT);
  pinMode (ENC_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_SW), encoder_button_pushed, FALLING);
  attachPinChangeInterrupt(ENC_CLK, encoder_read, CHANGE);
  /* Read Pin A
    Whatever state it's in will reflect the last position
  */
  enc_clk_last = digitalRead(ENC_CLK);
  previousMillisEncoder = 0;
  init_encoder_position(0);
}
