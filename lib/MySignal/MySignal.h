/**
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * A simple helper library for the builtin LED on ESP8266 devices
 * or any board with a signal LED actually
 * 
 * */

#ifndef MySignal_h
#define MySignal_h

#include "Arduino.h"

class MySignal
{
private:
    unsigned long _lastLED = 0;
    uint8_t _ledpin;
    bool _on;
    bool _off;

public:
    void activate(uint8_t ledpin, bool on, bool off); // enable pin ledport as output and define on and off levels
    void flash();                                     // invert the state of the output pin
    void on();                                        // set the output pin to LED on (as defined high or low)
    void off();                                       // set the output pin to LED off (as defined low or high)
    void check();                                     // see if the LED was already on for a predefined time; if so, turn it off
};
#endif