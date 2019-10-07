uint8_t brite = 2; // display brightness level 0-7

volatile unsigned long thisTick = 0; // gets set to millis() at the beginning of each loop()

// translate weekdays as you see fit, this is german ... runs with day keys from 1-7 and month 1-12
// key zero is not used.
char wd[8][3] = {"XX", "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"}; 
char mon[13][4] = {"XXX", "Jan", "Feb", "Mar", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"};

// various input/output pins

#define BUTTON D3
#define INTERNAL_LED D4
#define panelCS D8
#define MY_1HZ_INPUT D0
#define MY_I2C_SDA D2
#define MY_I2C_SCL D1

#define BUTTON_TIME_LONG 1000   // seconds for holding for menu
#define BUTTON_TIME_LONGER 3000 // seconds for holding for brightness cycling

uint8_t webserverOn = false;
uint8_t clockStyle = 0;
#define CLOCKSTYLE_MAX 6

union IntToTwo { 
int16_t INT;
uint8_t bytes[2];
};

IntToTwo scrollDelay;

unsigned long stepDisplay = 100; // millis between display updates
unsigned long holdDisplay = 0;   // optional absolute hold delay in ms to suppress regular countdown updates (is reduced by stepDisplay every loop)


#define MYRTCRAM_MODE 8            // RTC RAM address of our mode byte
#define MYRTCRAM_BRITE 9           // RTC RAM address of our brightness byte
#define MYRTCRAM_COUNTDOWN 10      // RTC RAM address of countdown value ( stored as char[14] )
#define MYRTCRAM_COUNTDOWN_SIZE 14 // RTC RAM address of countdown value ( stored as char[14] )
#define MYRTCRAM_CLOCKSTYLE 25     // RTC RAM address of countdown value
#define MYRTCRAM_SCROLLDELAY 26     // RTC RAM address of scroll delay (2 bytes for 16-bit value)

#define SCROLLER 0 // display mode
#define COUNTDOWN 1
#define CLOCK 2
#define CONFIG 3
#define COUNTDOWN_END 4
uint8_t displaymode = SCROLLER;
uint8_t truemode = SCROLLER;
uint8_t lastmode = SCROLLER;

typedef struct
{
    int16_t days;
    int16_t hours;
    int16_t minutes;
    int16_t seconds;
} myLongtime;

myLongtime countdown = {1000, 0, 0, 0}; // default

unsigned long stepKeys = 10; // how often the input is polled for debouncing

// Dot Matrix Display Strip

int numberOfHorizontalDisplays = 12;
int numberOfVerticalDisplays = 1;
byte panelRotation = 1; // 0-3

// Texts

char m_boot[] = "System BOOT";
char m_fsok[] = "Filesys OK";
char m_fserr[] = "Filesys ERR";
char m_info[] = "Short press to cycle display. Long press to activate WiFi config. Longer hold to cycle brightness.";
char m_i2c[] = "I2C Communication error with clock module";
char m_clock[] = "Clock Error! Check battery and re-run config!";

#define SCROLLER_BUFFER_SIZE 2048
char scrollerbuffer[1 + SCROLLER_BUFFER_SIZE] = "No config found.\nLong press button to start config.\nThis message will repeat.\n###########"; // this can contain multiple lines separated by CR
