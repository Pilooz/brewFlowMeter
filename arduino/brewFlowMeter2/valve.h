#include <arduino.h>

class Valve {
  public:
  Valve(int pin);
  void open();
  void close();
  int status();
  private:
  int _pin;
  int _status = LOW;
};

