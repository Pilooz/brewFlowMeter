#ifndef VALVE_H
#define VALVE_H

#include <arduino.h>

class Valve {
  public:
  Valve(uint8_t pin);
  void open();
  void close();
  void test();
  int status();
  private:
  uint8_t _pin;
  uint8_t _status = LOW;
};

#endif
