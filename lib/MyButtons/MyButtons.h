#ifndef MyButtons_h
#define MyButtons_h

#include "Arduino.h"

#define MYBUTTONS_PRESS_NONE 0
#define MYBUTTONS_RELEASE_SHORT 1
#define MYBUTTONS_HOLD_LONG 2
#define MYBUTTONS_RELEASE_LONG 3
#define MYBUTTONS_HOLD_LONGER 4
#define MYBUTTONS_RELEASE_LONGER 5
/*






Button debouncer logic and button mode detection
 */


class MyButtons
{
    private:
    int _buttonPin; // pin to which the button is connected (pull-down)
    int _buttonLong; // Amount of ms to consider it "long press"
    int _buttonLonger; // Amount of ms to consider it "even longer press" and skip over button_long state
    unsigned long _deltaTicks;

    public:
    MyButtons( int buttonPin, int longTicks, int longerTicks, int deltaTicks ); // init
    unsigned long check(); // call it periodically with time distance of deltaTicks for exact response
    int buttonMode;
    bool release_short(); // button was just released after being held down for time < longTicks
    bool hold_long(); // button is still being held down for longTicks < time < longerTicks
    bool release_long(); // button was just released after being held down for longTicks < time < longerTicks
    bool hold_longer(); // button is still being held down for a time > longerTicks
    bool release_longer(); // button was just release after being held down for time > longerTicks


};

#endif