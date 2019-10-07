#include "Arduino.h"
#include "MySignal.h"

#define INTERNAL_LED D4 // D4 = LED eingebaut, invertiert - LOW = EIN
unsigned long lastLED;  // für nachleuchtende Signal-LED

void MySignal::activate( uint8_t ledpin, bool on, bool off ) {
    _ledpin = ledpin;
    _on = on;
    _off = off;
    pinMode(_ledpin, OUTPUT);
}

void MySignal::flash()
{
    digitalWrite(_ledpin, !digitalRead(_ledpin));
}

void MySignal::on()
{
    digitalWrite(_ledpin, _on);
    _lastLED = millis();
}

void MySignal::off()
{
    digitalWrite(_ledpin, _off);
}

void MySignal::check()
{
    if (digitalRead(_ledpin) == _off)
    {
        return;
    }
    if (millis() - _lastLED > 400)
    {
        off();
    }
}
