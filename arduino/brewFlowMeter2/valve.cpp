
Valve::Valve(int pin) {
  _pin = pin;
  _status = LOW;
}

int Valve::status() {
  return _status;
}

void Valve::open() {
  if (_status == LOW) {
    //flw_read(true);
    digitalWrite(VLV, HIGH);
    _status = HIGH;
  }
}

void Valve::close() {
  if (_status == HIGH) {
    //flw_read(false);
    digitalWrite(VLV, LOW);
    _status = LOW;
  }  
}

