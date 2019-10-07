#include "Arduino.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

#include "MyMatrix.h"

// init the properties
MyMatrix::MyMatrix(int8_t pCS, int8_t numHor, int8_t numVert)
{
  _hell = 2;
  _dispHor = numHor;
  _dispVert = numVert;
  _offset = 0;
  scbCursor = 0;
  _maxX = _dispHor * 8;
  matrix = new Max72xxPanel(pCS, numHor, numVert); // matrix is a pointer, so use it with matrix->function()
}

void MyMatrix::Fill( bool f ) {
  matrix->fillScreen(f);
}

void MyMatrix::setX(int16_t x)
{
  _offset = x;
}

// process the internal timer; if ready shift the output of the display
// ( clear pixelbuffer then output the textbuffer using the new offset )
void MyMatrix::ShowScroll()
{
  if (_nextStep - millis() >= _stepDelay)
  {
    _nextStep = millis() + _stepDelay;
    _offset--;
    Show(0);
  }
}

// Prepare Scroll Mode: Offset is set to right border, and delay to stepDelay milliseconds
void MyMatrix::SetScroll(unsigned long stepDelay)
{
  _offset = _maxX;
  _stepDelay = stepDelay;
}

// Alter Scroller Delay while its running, but don't restart text
void MyMatrix::SetScrollDelay( unsigned long stepDelay )
{
  _stepDelay = stepDelay;
  _nextStep = 0;
}

void MyMatrix::SetAfterScroll(void (*functionPointer)())
{
  _functionPointer = functionPointer;
  /*
  Serial.println( "Function Pointer Value is ");
  Serial.println( (long) &_functionPointer );
  */
}

void MyMatrix::ClearAfterScroll() {
  _functionPointer = 0;
}

// check if the X coordinate is still within visible frame
void MyMatrix::wrapscroller(int16_t rX)
{
  if (rX <= 0)
  {                  // last character has left the visible area
    _offset = _maxX; // so let's restart at the right edge
    if( _functionPointer != 0 ) {
      _functionPointer(); // call Callback
    }
  }
}

// store the brightness for internal use
void MyMatrix::SetLevel(uint8_t hell)
{
  _hell = hell;
}

// write the pixel buffer data into the display
void MyMatrix::display()
{
  matrix->write();
}

void MyMatrix::runInit(byte panelRotation)
{
  for (uint8_t i = 0; i < (_dispHor * _dispVert); i++)
  {
    matrix->setRotation(i, panelRotation);
  }

  /* Teststreifen */
  matrix->fillScreen(LOW);

  for (int16_t i = 0; i < matrix->width(); i++)
  {
    matrix->drawLine(i, 0, i, matrix->height(), HIGH);
    matrix->write();
    delay(20);
  }
}

// store the text to display
void MyMatrix::SetTextBuffer(char *freitext)
{
  strlcpy(_textbuffer, freitext, sizeof(_textbuffer));
}

// copy chars to the display (no special stuff)
void MyMatrix::Show(unsigned int del)
{

  int16_t xpos = _offset;

  matrix->fillScreen(LOW);
  matrix->setCursor(0, 0);

  matrix->setIntensity(_hell);

  if (del != 0)
  { // Blinken
    matrix->write();
    delay(del);
  }

  for (uint16_t i = 0; i < sizeof(_textbuffer); i++)
  {
    if (_textbuffer[i] == 0)
    { // End of line
      break;
    }
    if (xpos > _maxX)
    {
      break;
    }
    if (xpos > -6)
    {
      matrix->drawChar(xpos, 0, _textbuffer[i], HIGH, LOW, 1);
    }
    xpos += 6;
  }

  wrapscroller(xpos);

  matrix->write(); // Send bitmap to display
}

// copy chars of SetTextBuffer() to the display
// including scroll
// using reduced width for known-thinner chars
// and make : flash ever 0.5 seconds
void MyMatrix::ShowCompact(unsigned int del)
{

  int16_t xpos = _offset;

  matrix->fillScreen(LOW);
  matrix->setCursor(0, 0);

  matrix->setIntensity(_hell);

  if (del != 0)
  { // option to quickly blank the display ... do not overdo it, it uses the ugly DELAY
    matrix->write();
    delay(del);
  }

  for (unsigned int i = 0; i < sizeof(_textbuffer); i++)
  {
    if (_textbuffer[i] == 0)
    { // End of line?
      break;
    }
    if (xpos > _maxX)
    { // beyond right display edge? good enough...
      break;
    }
    if (_textbuffer[i] == ':' || _textbuffer[i] == '.' || _textbuffer[i] == ' ' )
    {
      xpos -= 2;
    }
    if (_textbuffer[i] == 'i' || _textbuffer[i] == 'l' || _textbuffer[i] == '(' || _textbuffer[i] == ')')
    {
      xpos -= 1;
    }

    // Flashing seconds colon

    if (millis() % 1000 > 500 && _textbuffer[i] == ':')
    {
      // show colon only 0.5 out of every 1 second
    }
    else
    {
      if (xpos > -6)
      { // only print if char is possibly inside frame
        matrix->drawChar(xpos, 0, _textbuffer[i], HIGH, HIGH, 1);
      }
    }
    if (_textbuffer[i] == ':')
    {
      xpos -= 2;
    }
    if (_textbuffer[i] == '.')
    {
      xpos -= 1;
    }
    if (_textbuffer[i] == 'i' || _textbuffer[i] == 'l' || _textbuffer[i] == '(' || _textbuffer[i] == ')')
    {
      xpos -= 1;
    }

    xpos += 6;
  }

  wrapscroller(xpos);

  matrix->write(); // Send bitmap to display
}
