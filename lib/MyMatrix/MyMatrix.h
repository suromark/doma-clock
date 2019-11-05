#ifndef MyMatrix_h
#define MyMatrix_h

#include "Arduino.h"
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Max72xxPanel.h"

class MyMatrix
{
private:
    int8_t _dispHor; // number of 8x8 displays stacked along X
    int8_t _dispVert; // number of 8x8 displays stacked along Y
    int16_t _offset = 0; // pixels shift for scrolling text
    uint16_t _maxX = 8; // total number of pixels in X
    uint16_t _buffer_pix_width = 0; // the width of the current text buffer in pixels to center non-proportional output (width is not calculated beyond _maxX)
    uint16_t _buffer_pix_width_prop = 0; // the width of the current text buffer in pixels to center proportional output (width is not calculated beyond _maxX)

    unsigned long _stepDelay = 0; // ms to wait between scroll steps 
    unsigned long _nextStep = 0; // timestamp for expected next scroll action
    void wrapscroller( int16_t rX ); // Check-Routine at end of each text output (rX is given planned screen position of next letter )
    void (* _functionPointer)(); // a function pointer storage (or zero if no callback wanted), called as the last character of the scroller exits left
    int16_t proportional_compensate_pre( char c ); // return a pixel compensation value to retract X by before printing char c
    int16_t proportional_compensate_post( char c ); // return a pixel compensation value to retract X by AFTER printing char c

public:
    MyMatrix(int8_t pCS, int8_t numHor, int8_t numVert);
    char _textbuffer[512]; // should suffice for most use cases, can be set bigger at the cost of free RAM
    void SetTextBuffer(char *sometext); // copy any text to _textbuffer for display
    void setRotationFromFile(); // Read SPIFFS file orient.txt which needs to contain rotation values (as chars 0,1,2,3)
    void ClearTextBuffer(); // set the first char of _textbuffer to \0
    void RecalcCenter(); // Calculate the pixel width of textbuffer for prop and non-prop display
    void SetLevel(uint8_t hell); // Set the brightness (range 0-7 is adequate, 0-31 is the Max7219 maximum)
    void Show(unsigned int del); // display the _textbuffer on the LED (using _offset pixels as starting point)
    void ShowCentered( unsigned int del); // display the _textbuffer centered, non-prop
    void ShowCompact(unsigned int del); // display the _textbuffer, prop, using _offset
    void ShowCompactCentered(unsigned int del); // display the _textbuffer centered, proportional
    void SetAfterScroll( void (* functionPointer)() ); // tell what function to call once the last char has scrolled offscreen
    void ClearAfterScroll(); // clear the AfterScroll function pointer
    void SetScroll( unsigned long stepDelay ); // begin a new scrolling text with a certain step delay (ms)
    void setX( int16_t x );
    void ShowScroll(); // check if Delay is up, if so: move the display by 1 pixel and repeat
    void runInit(byte panelRotation); // define the rotation 0-3 of panels (hardware dependant)
    void Fill( bool f ); // Fill the whole display with On or Off 
    void display(); // write the internal buffer to the actual dot matrix ICs
    void SetScrollDelay( unsigned long stepDelay ); // change the ms between 1px scroll steps
    void reconnect(); // Set the display to standby, then immediately wake it again (hopefully behaves like a reset)
    uint16_t scbCursor = 0; // a helper associated with the main scrollertext buffer but placed here for convenience
    Max72xxPanel *matrix; // the matrix driver class
    uint8_t _hell = 0; // brightness level storage
    unsigned long _millis = 0; // a helper buffer to avoid both repeated calls to millis() inside loops and multiple storages for the same functionality
};

#endif