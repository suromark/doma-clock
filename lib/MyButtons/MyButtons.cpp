#include "Arduino.h"
#include "MyButtons.h"

MyButtons::MyButtons(int buttonPin, int longTicks, int longerTicks, int deltaTicks)
{
    _buttonPin = buttonPin;
    _buttonLong = longTicks;
    _buttonLonger = longerTicks;
    _deltaTicks = deltaTicks;
}

// returns the accumulates button press time and modifies the buttonMode status according to time and press/release
unsigned long MyButtons::check()
{

    static byte b_flow = 0;                 // used as debouncing shift register
    static unsigned long buttonPressed = 0; // internal number of ticks the button is being held down

    b_flow = b_flow << 1 | (digitalRead(_buttonPin) == LOW ? 1 : 0);

    if ((b_flow & 0b10011111) == 0b10011111)
    {
        buttonPressed = buttonPressed + _deltaTicks;
    }

    if( buttonPressed >= _buttonLong ) {
        buttonMode = MYBUTTONS_HOLD_LONG;
    }

    if (buttonPressed >= _buttonLonger)
    {
        buttonMode = MYBUTTONS_HOLD_LONGER;
    }

    if ((b_flow & 0b10011111) == 0) // evaluation of a press happens only on release time
    {
        buttonMode = MYBUTTONS_PRESS_NONE;

        if (buttonPressed > 0)
        {

            if (buttonPressed < _buttonLong)
            {
                buttonMode = MYBUTTONS_RELEASE_SHORT;
            }
            else
            {
                if (buttonPressed >= _buttonLonger)
                {
                    buttonMode = MYBUTTONS_RELEASE_LONGER;
                }
                else
                {
                    buttonMode = MYBUTTONS_RELEASE_LONG;
                }
            }
        }
        buttonPressed = 0;
    }

    return buttonPressed;
}

// button was just released after being held down for time < longTicks
bool MyButtons::release_short()
{
    if (buttonMode == MYBUTTONS_RELEASE_SHORT)
    {
        return true;
    }

    return false;
}

// button was just released after being held down for longTicks < time < longerTicks
bool MyButtons::release_long()
{
    if (buttonMode == MYBUTTONS_RELEASE_LONG)
    {
        return true;
    }
    return false;
}

// button is still being held down for a time > longerTicks
bool MyButtons::hold_long()
{
    if (buttonMode == MYBUTTONS_HOLD_LONG )
    {
        return true;
    }
    return false;
}

// button is still being held down for a time > longerTicks
bool MyButtons::hold_longer()
{
    if (buttonMode == MYBUTTONS_HOLD_LONGER)
    {
        return true;
    }
    return false;
}

bool MyButtons::release_longer()
{
    if (buttonMode == MYBUTTONS_RELEASE_LONGER)
    {
        return true;
    }
    return false;
}