#include <ESP8266WebServer.h>
#include <FS.h>

IPAddress apIP(10, 20, 30, 40);      // Captive Portal IP
ESP8266WebServer mycp_webServer(80); // Generic Web Server

void mycp_config_dump();
void mycp_scroller_dump();
void my_webserver_start();
void mycp_parse_set();
bool my_spiffs_setup();
bool handleFileRead(String path);
String getContentType(String filename);

char c_chipstring_hex[9]; // Hex-String of ChipID (typ. 8 chars)
char apName[30];
char apPass[30];

/*







   Config Server Setup (using AP mode)
*/
void my_configserver_setup()
{
    /*
    Get My ChipID (6 hex digits) + last 2 hex chars of FlashID ( Homie default )
  */
    char flashChipId[6 + 1]; // Buffer
    snprintf(flashChipId, 6, "%06x", ESP.getFlashChipId());
    snprintf(c_chipstring_hex, sizeof c_chipstring_hex, "%06x%s", ESP.getChipId(), flashChipId + strlen(flashChipId) - 2);
    Serial.print("Chipstring generated: ");
    Serial.println(c_chipstring_hex);

    snprintf(apName, sizeof apName, "MClock-%s", c_chipstring_hex);
    // snprintf(apPass, sizeof apPass, "%s", c_chipstring_hex);
    snprintf(apPass, sizeof apPass, "%08d", ESP.getChipId());

    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(apName, apPass);
    Serial.println("Use IP Address");
    Serial.println(apIP);

    my_webserver_start();
}

/*




WiFi Server Loop 
*/
void my_wifi_loop()
{
    /*
     process pending web server requests
  */
    mycp_webServer.handleClient();
}

/*






   Webserver Start/Setup (for AP and Client mode)
*/
void my_webserver_start()
{
    /*
    Declare server response functions
    */

    Serial.println("Webserver init");

    mycp_webServer.on("/config", mycp_config_dump);
    mycp_webServer.on("/set", mycp_parse_set);

    mycp_webServer.onNotFound([]() {                                  // If the client requests any URI
        if (!handleFileRead(mycp_webServer.uri()))                    // send it if it exists
            mycp_webServer.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
    });

    mycp_webServer.begin();
}

void my_webserver_stop()
{
    Serial.println("Webserver halt");
    mycp_webServer.stop();
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);
}

/*



   Filesystem Serve Function
*/

String getContentType(String filename)
{ // convert the file extension to the MIME type
    if (filename.endsWith(".txt"))
        return "text/plain";
    if (filename.endsWith(".htm"))
        return "text/html";
    else if (filename.endsWith(".html"))
        return "text/html";
    else if (filename.endsWith(".css"))
        return "text/css";
    else if (filename.endsWith(".js"))
        return "application/javascript";
    else if (filename.endsWith(".ico"))
        return "image/x-icon";
    return "text/plain";
}

bool handleFileRead(String path)
{ // send the right file to the client (if it exists)
    led.on();
    Serial.println("handleFileRead: " + path);
    if (path.endsWith("/"))
        path += "index.htm";                   // If a folder is requested, send the index file
    String contentType = getContentType(path); // Get the MIME type
    if (SPIFFS.exists(path))
    {                                                               // If the file exists
        File file = SPIFFS.open(path, "r");                         // Open it
        size_t sent = mycp_webServer.streamFile(file, contentType); // And send it to the client
        file.close();                                               // Then close the file again
        return true;
    }
    Serial.println("Translated: " + path);
    Serial.println("\tFile Not Found");
    return false; // If the file doesn't exist, return false
}

/*



   500 Server Error 
*/
void returnFail(String msg)
{
    mycp_webServer.sendHeader("Connection", "close");
    mycp_webServer.sendHeader("Access-Control-Allow-Origin", "*");
    mycp_webServer.send(500, "text/plain", msg + "\r\n");
}

/*





   Receive a parameter
   Schema: /set?key=A&val=B
*/

void mycp_parse_set()
{
    char inbuf[29];          // Buffer for splitting double datetime string YYYYMMDDHHIISSYYYYMMDDHHIISS
    char piece[5];           // datetime piece buffer, 4 chars max. required
    uint8_t mode;            // Buffer for Mode Value
    Chronos::DateTime curDT; // current DateTime
    led.on();
    if (!mycp_webServer.hasArg("key"))
        return returnFail("BAD ARGS KEY");
    if (!mycp_webServer.hasArg("val"))
        return returnFail("BAD ARGS VAL");

    /*
    Mode control command?
    */
    if (mycp_webServer.arg("key") == "mode")
    {
        mode = constrain(atoi(mycp_webServer.arg("val").c_str()), 0, 255);
        if (mode == SCROLLER)
        {
            switchToScroller();
            truemode = mode;
        }
        if (mode == COUNTDOWN)
        {
            switchToCountdown();
            truemode = mode;
        }
        if (mode == CLOCK)
        {
            switchToClock();
            truemode = mode;
        }
    }

    
    /*
    Clock Style control command?
    */
    if (mycp_webServer.arg("key") == "cstyle")
    {
        mode = constrain(atoi(mycp_webServer.arg("val").c_str()), 0, 255);
        if (mode <= CLOCKSTYLE_MAX)
        {
            clockStyle = mode;
            saveClockstyleToRam();
        }
    }

    /*
    Scroller Speed control command?
    */
    if (mycp_webServer.arg("key") == "sdelay")
    {
        mode = constrain(atoi(mycp_webServer.arg("val").c_str()), 0, 32767);
            scrollDelay.INT = mode;
            saveScrollDelayToRam();
            myma.SetScrollDelay( scrollDelay.INT );
    }

    /*
    Brightness control command?
    */
    if (mycp_webServer.arg("key") == "brite")
    {
        brite = constrain(atoi(mycp_webServer.arg("val").c_str()), 0, 7);
        saveBriteToRam();
    }

    /*
    Text segment for scroller memory? 
    (expects val to be 256 characters or less, and pos to be insert point within char array)
    */
   if( mycp_webServer.arg("key") == "scroller") {
       uint16_t pos = constrain( atoi( mycp_webServer.arg("pos").c_str()), 0, SCROLLER_BUFFER_SIZE-256 );
       strncpy( (char*) (scrollerbuffer + pos ), mycp_webServer.arg("val").c_str(), 256 );
   }

    /*
    Segmented transmission of scroller is done?
    (the client must send this at the end of the segmented transfer
    to trigger the save-to-flash action)
    */
   if( mycp_webServer.arg("key") == "sc_ended") {
       switchToScroller();
       save_scrolltext();
       Serial.println( "Saved scrollerbuffer to SPIFFS");
   }

    /*
Clock Config Command?
*/
    if (mycp_webServer.arg("key") == "times")
    {
        // get the two dates into Chronos values
        snprintf(inbuf, 28, "%s", mycp_webServer.arg("val").c_str());
        // YYYY
        strncpy(piece, inbuf, 4);
        piece[4] = '\0';
        curDT.setYear(atoi(piece));
        // MM
        strncpy(piece, inbuf + 4, 2);
        piece[2] = '\0';
        curDT.setMonth(atoi(piece));
        // DD
        strncpy(piece, inbuf + 6, 2);
        piece[2] = '\0';
        curDT.setDay(atoi(piece));
        // HH
        strncpy(piece, inbuf + 8, 2);
        piece[2] = '\0';
        curDT.setHour(atoi(piece));
        // II
        strncpy(piece, inbuf + 10, 2);
        piece[2] = '\0';
        curDT.setMinute(atoi(piece));
        // SS
        strncpy(piece, inbuf + 12, 2);
        piece[2] = '\0';
        curDT.setSecond(atoi(piece));

        // set the RTC clock to curDT
        Rtc.SetDateTime(RtcDateTime(curDT.year(), curDT.month(), curDT.day(), curDT.hour(), curDT.minute(), curDT.second()));
        // set the internal system time to curDT too
        Chronos::DateTime::setTime(curDT.year(), curDT.month(), curDT.day(), curDT.hour(), curDT.minute(), curDT.second());

        // now process the deadline string and update the global settings
        processDeadlineFromChar((char *)(inbuf + 14));

        // Store the countdown deadline as 14-char string in the RTC module RAM
        Rtc.SetMemory(MYRTCRAM_COUNTDOWN, (uint8_t *)(inbuf + 14), MYRTCRAM_COUNTDOWN_SIZE);
    }

// Config End Command?

    if (mycp_webServer.arg("key") == "done")
    {
        leaveConfig();
    }
    else
    {
        mycp_config_dump(); // default: dump all the current settings back to the client
    }
}

/*





   Send current config to client
*/

void mycp_config_dump()
{
    led.on();
    char c_buf[256];
    time_t thismoment;
    thismoment = now();
    snprintf(c_buf, sizeof c_buf,
             "{\"C\":\"%s\",\"M\":%d,\"BR\":%d,\"CS\":%d,\"SCD\":%d,\"rtc\":\"%04d%02d%02d%02d%02d%02d\",\"dedl\":\"%04d%02d%02d%02d%02d%02d\"}",
             c_chipstring_hex,
             truemode,
             brite,
             clockStyle,
             scrollDelay.INT,

             year(thismoment),
             month(thismoment),
             day(thismoment),
             hour(thismoment),
             minute(thismoment),
             second(thismoment),

             deadline.year(),
             deadline.month(),
             deadline.day(),
             deadline.hour(),
             deadline.minute(),
             deadline.second()

    );
    mycp_webServer.send(200, "application/json", c_buf);
}

/*



   A simplified Spiffs start -- without formatting/check since the external file system image upload must have taken care of that.

*/
bool my_spiffs_setup()
{

    Serial.println("SPIFFS mounting.");

    if (!SPIFFS.begin())
    {
        Serial.println("SPIFFS failed. Please upload a file system image first.");
        return false;
    }
    return true;
}

