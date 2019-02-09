int pinA = 3;  // Connected to CLK on KY-040
int pinB = 4;  // Connected to DT on KY-040
int pinC = 5;  // Connected to SW on KY-040

int encoderPosCount = 0;
int pinALast;
int aVal, bVal;
boolean bCW;
long previousMillisEncoder = 0; 

void setup() {
  pinMode (pinA, INPUT);
  pinMode (pinB, INPUT);
  pinMode (pinC, INPUT_PULLUP);
  /* Read Pin A
    Whatever state it's in will reflect the last position
  */
  pinALast = digitalRead(pinA);
  Serial.begin (115200);
}

void loop() {
  aVal = digitalRead(pinA);
  long currentMillis = millis();
  if (aVal != pinALast && currentMillis - previousMillisEncoder > 50) { // Means the knob is rotating
    previousMillisEncoder = currentMillis;
    // if the knob is rotating, we need to determine direction
    // We do that by reading pin B.
    if (digitalRead(pinB) != aVal) {  // Means pin A Changed first - We're Rotating Clockwise
      encoderPosCount ++;
      bCW = true;
    } else {// Otherwise B changed first and we're moving CCW
      bCW = false;
      encoderPosCount--;
    }
    Serial.print ("Rotated: ");
    if (bCW) {
      Serial.println ("clockwise");
    } else {
      Serial.println("counterclockwise");
    }
    Serial.print("Encoder Position: ");
    Serial.println(encoderPosCount);

  }
  pinALast = aVal;

  // Button detection
  if (digitalRead(pinC) == LOW) {
    Serial.println(" => Button was pushed !"); 
    delay(300); //debounce
  }
}
