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
    unsigned long _stepDelay = 0; // ms to wait between scroll steps 
    uint16_t _maxX = 8; // total number of pixels in X
    unsigned long _nextStep = 0; // timestamp for expected next scroll action
    void wrapscroller( int16_t rX ); // Check-Routine am Ende einer Textausgabe (rX enthält Position des nächsten Buchstabens )
    void (* _functionPointer)(); // a pointer storage (or zero if no callback wanted)

public:
    MyMatrix(int8_t pCS, int8_t numHor, int8_t numVert);
    char _textbuffer[512]; // sollte für die meisten Zwecke ausreichen; kann höher konfiguriert werden auf Kosten des RAM
    void SetTextBuffer(char *freitext);
    void SetLevel(uint8_t hell);
    void Show(unsigned int del);
    void ShowCompact(unsigned int del);
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