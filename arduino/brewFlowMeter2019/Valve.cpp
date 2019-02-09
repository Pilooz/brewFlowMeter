#include "Valve.h"

Valve::Valve(uint8_t pin) {
  _pin = pin;
  pinMode(_pin, OUTPUT);
  _status = LOW;
  digitalWrite(_pin, _status);

}

int Valve::status() {
  return _status;
}

void Valve::open() {
  if (_status == LOW) {
    digitalWrite(_pin, HIGH);
    _status = HIGH;
  }
}

void Valve::close() {
  if (_status == HIGH) {
    digitalWrite(_pin, LOW);
    _status = LOW;
  }
}

void Valve::test() {
  open();
  delay(2000);
  close();
  delay(2000);
}
