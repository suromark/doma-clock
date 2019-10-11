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
    int16_t proportional_compensate_pre( char c );
    int16_t proportional_compensate_post( char c );

public:
    MyMatrix(int8_t pCS, int8_t numHor, int8_t numVert);
    char _textbuffer[512]; // should suffice for most use cases, can be set bigger at the cost of free RAM
    void SetTextBuffer(char *freitext);
    void RecalcCenter();
    void SetLevel(uint8_t hell);
    void Show(unsigned int del);
    void ShowCentered( unsigned int del);
    void ShowCompact(unsigned int del);
    void ShowCompactCentered(unsigned int del);
    void SetAfterScroll( void (* functionPointer)() );
    void ClearAfterScroll();
    void SetScroll( unsigned long stepDelay ); // begin a new scrolling text with a certain step delay (ms)
    void setX( int16_t x );
    void ShowScroll(); // check if Delay is up, if so: move the display by 1 pixel and repeat
    void runInit(byte panelRotation);
    void Fill( bool f );
    void display();
    void SetScrollDelay( unsigned long stepDelay );
    uint16_t scbCursor = 0; // a helper associated with the main scrollertext buffer but placed here for convenience
    Max72xxPanel *matrix;
    uint8_t _hell = 0;
};

#endif