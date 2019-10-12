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

void MyMatrix::Fill(bool f)
{
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
  _millis = millis();
  if (_nextStep - _millis >= _stepDelay)
  {
    _nextStep = _millis + _stepDelay;
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
void MyMatrix::SetScrollDelay(unsigned long stepDelay)
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

void MyMatrix::ClearAfterScroll()
{
  _functionPointer = 0;
}

// check if the X coordinate is still within visible frame
void MyMatrix::wrapscroller(int16_t rX)
{
  if (rX <= 0)
  {                  // last character has left the visible area
    _offset = _maxX; // so let's restart at the right edge
    if (_functionPointer != 0)
    {
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
    matrix->drawLine(i-2, 0, i-2, matrix->height(), LOW);
    matrix->write();
    delay(20);
  }
}

// store the text to display,
// calculate its width (from char 0 to xmax display width then give up) for optional align = center
// in both fixed and proportional mode
void MyMatrix::SetTextBuffer(char *freitext)
{
  strlcpy(_textbuffer, freitext, sizeof(_textbuffer)); //textcopy
  RecalcCenter();
}

void MyMatrix::ClearTextBuffer() {
  _textbuffer[0] = '\0';
}

void MyMatrix::RecalcCenter()
{
  int16_t xpos = 0;
  int16_t xpos_prop = 0;

  for (uint16_t i = 0; i < sizeof(_textbuffer); i++) // worst case range
  {
    if (_textbuffer[i] == 0)
    { // End of line
      _buffer_pix_width = xpos;
      _buffer_pix_width_prop = xpos_prop;
      break;
    }
    xpos += 6; // start of next char non-prop
    if (xpos < _maxX )
    {
      _buffer_pix_width = xpos; 
    } else {
      _buffer_pix_width = _maxX;
    }

    if( xpos_prop > _maxX ) {
      _buffer_pix_width_prop = _maxX;
      break; // if the proportional end is reached, then the non-prop has already done so too, so exit
    }
    
    xpos_prop = xpos_prop + 6 + proportional_compensate_pre(_textbuffer[i]) + proportional_compensate_post(_textbuffer[i]);
  }
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

// copy chars to the display, trying to center the output
// This will NOT call the wrapscroller function ever!
void MyMatrix::ShowCentered(unsigned int del)
{

  int16_t xpos = 0;

  if (_maxX > _buffer_pix_width)
  {
    xpos = (_maxX - _buffer_pix_width) >> 1;
  }

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

  matrix->write(); // Send bitmap to display
}

int16_t MyMatrix::proportional_compensate_pre(char c)
{
  if (c == ':' || c == '.' || c == ' ')
    return -2;

  if (c == 'i' || c == 'l' || c == '(' || c == ')')
    return -1;

  return 0;
}

int16_t MyMatrix::proportional_compensate_post(char c)
{
  if (c == ':')
    return -2;

  if ( c == '.' || c == 'k' || c == 'i' || c == 'l' || c == '(' || c == ')')
    return -1;

  return 0;
}

// copy chars of SetTextBuffer() to the display
// including scroll
// using reduced width for known-thinner chars
// and make : flash ever 0.5 seconds
void MyMatrix::ShowCompact(unsigned int del)
{

  int16_t xpos = _offset;

 _millis = millis();
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
    { // beyond right display edge? enough chars printed for now ...
      break;
    }

    xpos += proportional_compensate_pre(_textbuffer[i]);

    // Flashing seconds colon

    if (_millis % 1000 > 500 && _textbuffer[i] == ':')
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
    xpos += proportional_compensate_post(_textbuffer[i]);

    xpos += 6;
  }

  wrapscroller(xpos);

  matrix->write(); // Send bitmap to display
}

// copy chars of SetTextBuffer() to the display
// NOT SCROLLING but centered within display (if fits, otherwise left-start with overflow)
// using reduced width for known-thinner chars
// and make : flash ever 0.5 seconds
void MyMatrix::ShowCompactCentered(unsigned int del)
{

  int16_t xpos = 0;

_millis = millis();

  matrix->fillScreen(LOW);
  matrix->setCursor(0, 0);

  matrix->setIntensity(_hell);

  

  if (_buffer_pix_width_prop < _maxX)
  {
    xpos = (_maxX - _buffer_pix_width_prop) >> 1;
  }

  if (del != 0)
  { // option to quickly blank the display before showing the new output
    // ... do not overdo it, it uses the ugly DELAY
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
    { // beyond right display edge? enough chars printed for now ...
      break;
    }

    xpos += proportional_compensate_pre(_textbuffer[i]);

    // Flashing seconds colon

    if (_millis % 1000 > 500 && _textbuffer[i] == ':')
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
    xpos += proportional_compensate_post(_textbuffer[i]);

    xpos += 6;
  }

  matrix->write(); // Send bitmap to display
}
