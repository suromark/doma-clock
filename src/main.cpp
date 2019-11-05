/*
Wiring: Wemos D1 mini -> Max7219 LED 8x8 matrix display chain
    5 V -> Vcc
    GND -> GND
    D8 HSPI-CS -> CS
    D6 HSPI-MISO -> -- not used --
    D7 HSPI-MOSI -> Din
    D5 HSPI-CLK -> CLK

Wiring: Action Button
    D3 <- Switch <- GND

Wiring: Real Time Clock Module
    D1 I2C-SCL -> CLK
    D2 I2C-SDA -> SDA
    D0         <- SQ 1-second-beat


I prefer to add 500-ish Ohms resistors along all ESP I/O lines, 
just in case I mess up the coding 
and/or I accidentally plug in a microcontroller with pins already set to output
and/or the signal isn’t at 3.3 volts level, to limit the current.
*/

#include <Arduino.h>
#include "myglobals.h"

bool readFromRTC();
void save_scrolltext();
void load_scrolltext();
void processDeadlineFromChar(char *inbuf);
void doBrite();
void saveBriteToRam();
void saveClockstyleToRam();
void saveScrollDelayToRam();
void doNextMode();
void progressCountdown();
void doCountdownDisplay();
void doCountdown();
void switchToCountdown();
void doCountdownEnd();
void switchToCountdownEnd();
void doInput();
void leaveConfig();
void switchToConfig();
void doConfig();
void switchToClock();
void switchToClockKeepStyle();
void doClock();
void switchToScroller();
void doScroller();
void doScrollerLineChange();
void increaseTime();
void decreaseTime();
void doReconnect();
myLongtime sanitizeTime(myLongtime t);

#include "Chronos.h" // this includes also timelib.h

// a default value for the countdown target, it is later overwritten (hopefully) by the value from RTC memory
Chronos::DateTime deadline = {2030, 01, 01, 12, 0, 0};

/*
 I2C connection for real time clock module
*/

#define countof(a) (sizeof(a) / sizeof(a[0]))
#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS1307.h>
RtcDS1307<TwoWire> Rtc(Wire);

/*
Own stuff
*/
#include "MySignal.h"
MySignal led;

#include "MyMatrix.h"
MyMatrix myma = MyMatrix(panelCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

#include "MyButtons.h"
MyButtons mbut = MyButtons(BUTTON, BUTTON_TIME_LONG, BUTTON_TIME_LONGER, stepKeys);

#include "myweb.h"

// Wifistuff

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
WiFiClient espClient;

/*




*/
void setup()
{
    Serial.begin(74880);

    pinMode(BUTTON, INPUT_PULLUP);
    pinMode(MY_1HZ_INPUT, INPUT_PULLUP); // unlucky that D0 does not support interrupts, but eh ... it'll do

    led.activate(INTERNAL_LED, LOW, HIGH);

    myma.SetLevel(brite);        // Use a value between 0 and 15 for brightness, but 0-7 usually suffices and uses less power
    myma.runInit(panelRotation); // set rotation of panels (adjust file data/orient.txt to your modules' layout), and do a bit of boot deco
    myma.SetTextBuffer(m_boot);
    myma.ShowCompact(50);
    delay(500);

    Serial.println(ESP.getChipId(), 16);

    Serial.println("Wifi OFF");
    WiFi.persistent(false); // avoid FLASH wear-out?

    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);

    if (my_spiffs_setup() == false)
    {
        myma.SetTextBuffer(m_fserr);
        myma.ShowCompact(100);
        delay(10000);
        ESP.restart();
        delay(10000);
    }
    else
    {
        myma.setRotationFromFile();
        myma.SetTextBuffer(m_fsok);
        myma.ShowCompact(100);
        delay(500);
    }
    Wire.begin(MY_I2C_SDA, MY_I2C_SCL);

    if (0)
    {
        // init clock with fixed data
        Rtc.SetDateTime(RtcDateTime(2018, 10, 9, 8, 7, 6));
        Rtc.SetIsRunning(true);
    }

    myma.SetTextBuffer(m_info); // prepare generic scroller text

    readFromRTC();

    load_scrolltext();

    // recover the stored mode after initial scroll has passed
    if (displaymode == CLOCK)
    {
        myma.SetAfterScroll(switchToClockKeepStyle);
    }
    if (displaymode == COUNTDOWN)
    {
        myma.SetAfterScroll(switchToCountdown);
    }
    if (displaymode == SCROLLER)
    {
        myma.SetAfterScroll(switchToScroller);
    }

    myma.SetScroll(15);
    displaymode = SCROLLER;
    truemode = displaymode;
    lastmode = displaymode; // this prevents instant back-saving of this initial scroller mode to RTC RAM since there's no difference
}

/**
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * Hauptschleife
 * */
void loop()
{

    thisTick = millis();

    doReconnect();

    led.check();

    doInput();

    doBrite();

    if (displaymode == CONFIG)
    {
        doConfig();
    }
    else
    {
        truemode = displaymode; // this is to regularly update the "actual" display mode EXCEPT during active Config display
    }

    // Store into clock ram on mode change
    if (lastmode != truemode)
    {
        Rtc.SetMemory(MYRTCRAM_MODE, truemode);
        lastmode = truemode;
        Serial.print("Saving mode = ");
        Serial.println(truemode);
    }

    // Webserver loop
    if (webserverOn == true)
    {
        my_wifi_loop();
    }

    // Countdown processing
    progressCountdown();

    if (displaymode == SCROLLER)
        doScroller();

    if (displaymode == COUNTDOWN)
        doCountdown();

    if (displaymode == COUNTDOWN_END)
        doCountdownEnd();

    if (displaymode == CLOCK)
        doClock();
}

// toggle through regular display modes (NO CONFIG)
// can break out of config display to show the current settings as preview
void doNextMode()
{
    if (displaymode == SCROLLER)
    {
        switchToCountdown();
        return;
    }
    if (displaymode == COUNTDOWN || displaymode == COUNTDOWN_END)
    {
        switchToClock();
        return;
    }
    if (displaymode == CLOCK)
    {
        if (clockStyle < CLOCKSTYLE_MAX)
        {
            clockStyle++;
            saveClockstyleToRam();
            return;
        }
        else
        {
            switchToScroller();
            return;
        }
    }
    switchToClock(); // default action allows to hide/preview out of the config scroller
    return;
}

// propagate the global brightness level to the display every 200ms
void doBrite()
{
    static unsigned long tickBright = 0;

    if (thisTick > tickBright)
    {
        tickBright = thisTick + 200;
        myma.SetLevel(brite);
    }
}

// do a clock reset every minute in case one of the drivers locks up
void doReconnect()
{
    static unsigned long tickCheck = 0;

    if (thisTick > tickCheck)
    {
        tickCheck = thisTick + 5000;
        myma.reconnect();
    }
}

void switchToCountdown()
{
    myma.ClearAfterScroll();
    myma.ClearTextBuffer();
    displaymode = COUNTDOWN;
}

/*
run the button input polling functions 
*/
void doInput()
{

    static unsigned long nextKeys = 0; // the next millis() value to do button polling

    if (thisTick > nextKeys)
    {
        nextKeys = thisTick + stepKeys;
        mbut.check();
        if (mbut.hold_long())
        {
            led.on();
        }

        if (mbut.release_short()) // cycle through display modes
        {
            doNextMode();
        }

        if (mbut.release_long())
        {
            mbut.check();
            Serial.print("Config Mode ");
            if (webserverOn == true)
            {
                leaveConfig();
            }
            else
            {
                Serial.println("starting");
                switchToConfig();
                webserverOn = true;
            }
        }

        if (mbut.hold_longer())
        { // cycle display brightness after 3 seconds
            led.flash();

            brite = (thisTick >> 10) & 0b0111; // use 0-7 of millis() / 1024 (1/sec cyclic increase)

            if (brite != myma._hell)
            {
                saveBriteToRam();
            }

            myma.SetLevel(brite);
        }
    }
}

void leaveConfig()
{
    Serial.println("leaving Config");
    my_webserver_stop();
    webserverOn = false;
    displaymode = truemode;
    if (displaymode == CLOCK)
        switchToClock();
    if (displaymode == SCROLLER)
        switchToScroller();
    if (displaymode == COUNTDOWN || displaymode == COUNTDOWN_END)
        switchToCountdown();
}

void switchToConfig()
{
    myma.ClearAfterScroll();
    myma.SetScroll(30);
    webserverOn = true;
    my_configserver_setup();
    truemode = displaymode;
    displaymode = CONFIG;

    snprintf(myma._textbuffer,
             sizeof(myma._textbuffer),
             "CONFIG MODE - WiFi AP: %s - Password: %s - http://%s/ ",
             apName,
             apPass,
             apIP.toString().c_str());
}

void doConfig()
{
    myma.ShowScroll();
}

void doCountdown()
{

    static unsigned long nextDisplay = 0;

    if (thisTick > nextDisplay)
    {
        nextDisplay = thisTick + stepDisplay;
        if (holdDisplay > stepDisplay)
        {
            holdDisplay = holdDisplay - stepDisplay;
        }
        else
        {
            doCountdownDisplay();
        }
    }
}

void progressCountdown()
{
    static bool last_signal = 0;
    bool signal = 0;

    signal = digitalRead(MY_1HZ_INPUT);

    if (last_signal != signal) // change in signal
    {
        last_signal = signal;

        if (signal == HIGH)
        { // rising edge found

            if (countdown.days != 0 || countdown.hours != 0 || countdown.minutes != 0 || countdown.seconds != 0)
            {
                decreaseTime();
            }

            if (displaymode == COUNTDOWN && countdown.days == 0 && countdown.hours == 0 && countdown.minutes == 0 && countdown.seconds == 0)
            {
                switchToCountdownEnd();
            }
        }
    }
}

// Print the current countdown status: typically 12-13 chars + NULL in format: DDD HH:II:SS
void doCountdownDisplay()
{

    countdown = sanitizeTime(countdown); // no harm in over-doing but a few clock cycles

    snprintf(myma._textbuffer, sizeof(myma._textbuffer), "%03d %02d:%02d:%02d", countdown.days, countdown.hours, countdown.minutes, countdown.seconds);
    myma.setX(0);
    myma.ClearAfterScroll();
    myma.SetLevel(brite);
    myma.RecalcCenter();
    myma.ShowCompactCentered(0);
}

// does carry operations on countdown time structure in case one is negative or bigger than permitted
myLongtime sanitizeTime(myLongtime t)
{
    if (t.seconds < 0)
    {
        t.minutes = t.minutes - 1 + (t.seconds / 60);
        t.seconds = (60 + (t.seconds % 60)) % 60;
    }

    if (t.seconds >= 60)
    {
        t.minutes = t.minutes + (t.seconds / 60);
        t.seconds = t.seconds % 60;
    }

    if (t.minutes < 0)
    {
        t.hours = t.hours - 1 + (t.minutes / 60);
        t.minutes = (60 + (t.minutes % 60)) % 60;
    }

    if (t.minutes >= 60)
    {
        t.hours = t.hours + (t.minutes / 60);
        t.minutes = t.minutes % 60;
    }

    if (t.hours < 0)
    {
        t.days = t.days - 1 + (t.hours / 24);
        t.hours = (24 + (t.hours % 24)) % 24;
    }

    if (t.hours >= 24)
    {
        t.days = t.days + (t.hours / 24);
        t.hours = t.hours % 24;
    }

    if (t.days < 0)
    {
        t.days = t.days % 9999;
    }

    if (t.days >= 9999)
    {
        t.days = t.days % 9999;
    }

    return t;
}

void decreaseTime()
{
    countdown.seconds--;
    countdown = sanitizeTime(countdown);
}

void increaseTime()
{
    countdown.seconds++;
    countdown = sanitizeTime(countdown);
}

void switchToCountdownEnd()
{
    displaymode = COUNTDOWN_END;
    myma.ClearAfterScroll();
    myma.Fill(0);
    myma.display();
}

void doCountdownEnd()
{
    static unsigned long last = 0;
    static bool mark = false;
    if (millis() - last >= 500) // this only runs every 0.5 seconds
    {
        last = millis();
        mark = !mark;
        myma.Fill(mark);
        myma.display();
    }
}

void switchToClockKeepStyle()
{
    displaymode = CLOCK;
    myma.ClearAfterScroll();
    myma.setX(0);
}

void switchToClock()
{
    clockStyle = 0;
    switchToClockKeepStyle();
}

void doClock()
{
    static unsigned long lastDisplay = 0;
    if (thisTick - lastDisplay >= 250)
    {
        lastDisplay = thisTick;
        // Style 0
        snprintf(myma._textbuffer, sizeof(myma._textbuffer), "%02d. %s %02d:%02d", day(), wd[weekday()], hour(), minute());

        if (clockStyle == 1)
            snprintf(myma._textbuffer, sizeof(myma._textbuffer), "%02d:%02d:%02d", hour(), minute(), second());

        if (clockStyle == 2)
            snprintf(myma._textbuffer, sizeof(myma._textbuffer), "%02d:%02d", hour(), minute());

        if (clockStyle == 3)
            snprintf(myma._textbuffer, sizeof(myma._textbuffer), "%s %02d:%02d:%02d", wd[weekday()], hour(), minute(), second());

        if (clockStyle == 4)
            snprintf(myma._textbuffer, sizeof(myma._textbuffer), "%04d-%02d-%02d %02d:%02d",
                     year(), month(), day(), hour(), minute());

        if (clockStyle == 5)
            snprintf(myma._textbuffer, sizeof(myma._textbuffer), "%s %02d %s %02d:%02d:%02d",
                     wd[weekday()], day(), mon[month()], hour(), minute(), second());

        if (clockStyle == 6)
            snprintf(myma._textbuffer, sizeof(myma._textbuffer), "%04d %s %02d  %02d:%02d",
                     year(), mon[month()], day(), hour(), minute());

        myma.RecalcCenter();
        myma.ShowCompactCentered(0);
    }
}

void switchToScroller()
{
    displaymode = SCROLLER;
    myma.scbCursor = 0;
    doScrollerLineChange();                    // fill the scroller textbuffer
    myma.SetAfterScroll(doScrollerLineChange); // and repeat it again once the line is shown
    myma.SetScroll(scrollDelay.INT);
}

// fill the matrix text buffer with the next line of the scroller text buffer
// cycle to beginning if needed
void doScrollerLineChange()
{

    char c;

    led.on();

    for (int i = 0; i < (int)(sizeof myma._textbuffer) - 1; i++)
    {
        c = scrollerbuffer[myma.scbCursor + i];
        myma._textbuffer[i] = c;

        if (c == '\0' && i == 0)
        {                          // found the absolute end of the buffer!
            myma.scbCursor = 0;    // jump to the beginning
            c = scrollerbuffer[0]; // take that character instead
            myma._textbuffer[i] = c;
        }

        if (c == '\0')
        {                       // stop reading, absolute end reached!
            myma.scbCursor = 0; // begin again next time
            break;
        }

        if (c == '\xA' || c == '\xD')
        {                                            // reached a line break
            myma.scbCursor = myma.scbCursor + i + 1; // memorize position-after-that
            myma._textbuffer[i] = '\0';              // mark as char end
            break;
        }
    }
}

void doScroller()
{
    myma.ShowScroll();
}

bool readFromRTC()
{
    bool chck = false; // generic boolean buffer
    RtcDateTime theNow;
    char inbuf[MYRTCRAM_COUNTDOWN_SIZE + 1];

    Rtc.SetSquareWavePin(DS1307SquareWaveOut_1Hz);
    Rtc.SetIsRunning(true);

    chck = Rtc.IsDateTimeValid();

    if (Rtc.LastError() != 0)
    {
        // we have a communications error
        // see https://www.arduino.cc/en/Reference/WireEndTransmission for
        // what the number means
        Serial.println(m_i2c);
        Serial.println(Rtc.LastError());
        strncpy(myma._textbuffer, m_i2c, sizeof(myma._textbuffer));
        return false;
    }

    if (!chck)
    {
        Serial.println(m_clock);
        strncpy(myma._textbuffer, m_clock, sizeof(myma._textbuffer));
        return false;
    }

    if (chck && Rtc.LastError() == 0)
    {
        Serial.println("Retrieving time from RTC");
        // Set internal clock to chip clock
        theNow = Rtc.GetDateTime();
        setTime(theNow.Epoch32Time());
    }
    else
    {
        Serial.println(m_i2c);
        strncpy(myma._textbuffer, m_i2c, sizeof(myma._textbuffer));
        return false;
    }

    // read the parameters stored in the clock RAM

    uint8_t m = Rtc.GetMemory(MYRTCRAM_MODE);
    displaymode = constrain(m, 0, 2); // only modes 0-2 are applicable

    Serial.print("Stored mode = ");
    Serial.println(m);

    // Brightness
    m = Rtc.GetMemory(MYRTCRAM_BRITE);
    brite = constrain(m, 0, 7); // Brightness limits ore 0-7

    Serial.print("Stored brightness = ");
    Serial.println(m);

    // Scroller Delay (2-byte)
    Rtc.GetMemory(MYRTCRAM_SCROLLDELAY, scrollDelay.bytes, 2);

    Serial.print("Stored ScrollDelay = ");
    Serial.println(scrollDelay.INT);

    // Clockstyle
    m = Rtc.GetMemory(MYRTCRAM_CLOCKSTYLE);
    clockStyle = constrain(m, 0, CLOCKSTYLE_MAX); // Clockstyle limit

    Serial.print("Stored ClockStyle = ");
    Serial.println(clockStyle);

    // Countdown DateTime String
    Rtc.GetMemory(MYRTCRAM_COUNTDOWN, (uint8_t *)inbuf, MYRTCRAM_COUNTDOWN_SIZE); // pass inbuf as pointer to the first byte in the char array
    inbuf[MYRTCRAM_COUNTDOWN_SIZE] = '\0';

    processDeadlineFromChar(inbuf);

    Serial.println("RAM says ");
    Serial.println(inbuf);

    //
    return true;

    //
}

// process a string of YYYYMMDDHHIISS into the deadline global variable
void processDeadlineFromChar(char *inbuf)
{

    Serial.println("Processing char values");
    Serial.println(inbuf);

    char piece[5];
    // YYYY
    strncpy(piece, inbuf + 0, 4);
    piece[4] = '\0';
    deadline.setYear(atoi(piece));
    // MM
    strncpy(piece, inbuf + 4, 2);
    piece[2] = '\0';
    deadline.setMonth(atoi(piece));
    // DD
    strncpy(piece, inbuf + 6, 2);
    piece[2] = '\0';
    deadline.setDay(atoi(piece));
    // HH
    strncpy(piece, inbuf + 8, 2);
    piece[2] = '\0';
    deadline.setHour(atoi(piece));
    // II
    strncpy(piece, inbuf + 10, 2);
    piece[2] = '\0';
    deadline.setMinute(atoi(piece));
    // SS
    strncpy(piece, inbuf + 12, 2);
    piece[2] = '\0';
    deadline.setSecond(atoi(piece));
    // calculate difference to the current time and set countdown
    Chronos::DateTime curDT = Chronos::DateTime::now();
    Chronos::Span::Absolute theDiff = deadline - curDT;
    if (curDT > deadline)
    {
        countdown = {0, 0, 0, 0};
    }
    else
    {
        countdown.days = (int16_t)theDiff.days();
        countdown.hours = (int16_t)theDiff.hours();
        countdown.minutes = (int16_t)theDiff.minutes();
        countdown.seconds = (int16_t)theDiff.seconds();
    }
}

void saveBriteToRam()
{
    Rtc.SetMemory(MYRTCRAM_BRITE, brite);
    Serial.print("Saving Brightness = ");
    Serial.println(brite);
}

void saveClockstyleToRam()
{
    Rtc.SetMemory(MYRTCRAM_CLOCKSTYLE, clockStyle);
    Serial.print("Saving Clockstyle = ");
    Serial.println(clockStyle);
}

void saveScrollDelayToRam() // 2-byte value!
{
    Rtc.SetMemory(MYRTCRAM_SCROLLDELAY, scrollDelay.bytes[0]);     // one byte
    Rtc.SetMemory(MYRTCRAM_SCROLLDELAY + 1, scrollDelay.bytes[1]); // other byte
    Serial.print("Saving ScrollDelay = ");
    Serial.println(scrollDelay.INT);
}

void load_scrolltext()
{
    size_t bytesRead;
    File file = SPIFFS.open("/scroll.txt", "r");
    if (file)
    {
        bytesRead = file.readBytes(scrollerbuffer, SCROLLER_BUFFER_SIZE);
        if (bytesRead >= 0 && bytesRead <= SCROLLER_BUFFER_SIZE)
        {
            scrollerbuffer[bytesRead] = '\x0';
        }
        file.close();
    }
    else
    {
    }
}

/*





   Save scrolltext settings into flash memory
*/

void save_scrolltext()
{
    size_t endpoint = SCROLLER_BUFFER_SIZE;
    File file = SPIFFS.open("/scroll.txt", "w");
    if (file)
    {
        Serial.println("Saving to Flash...");
        // find first zero value (end of string) - basically a strlen with a built-in upper limit;

        for (size_t i = 0; i < SCROLLER_BUFFER_SIZE; i++)
        {
            if (scrollerbuffer[i] == '\0')
            {
                endpoint = i;
                break;
            }
        }
        file.write((uint8_t *)scrollerbuffer, endpoint);
        file.close();
    }
    else
    {
        Serial.println("Error Saving");
    }
}
