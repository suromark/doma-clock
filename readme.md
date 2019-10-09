# DoMa-Clock

The Dot Matrix Clock thing by suromark. It's a countdown clock, a regular clock, and a text scroller. Uses MAX7219 8x8 LED displays driven by an ESP8266 microcontroller to do its thing. It uses PlatformIO and various libraries (check their license info).

It's also most likely a hodgepodge of coding sins, but hey ... everyone has to start somewhere.

**NOTE: The index.htm page of the built-in config web server is stored as SPIFFS file, so please do not just flash the program binary but also run PlatformIO's "Upload File System image" to make sure the Flash memory of the ESP is properly initialized.**

## Bill of materials

- 1x Wemos D1 mini ESP-8266 microcontroller
- 3x MAX7219 8x8 LED Matrix strip module (4 in a row pre-connected on a PCB)
- 1x DS1307 RTC module with battery and EEPROM
- Push Button (Normally Open type)
- Resistors 560 Ω (optional, to protect the ESP I/O pins against accidental reverse flow)
- Resistors 40-68 kΩ (optional, to modify the LED matrix current from its far too high default configuration)
- Various wires and connectors (if desired)
- Power supply 5 volts, max. ampere rating depends on whether you've throttled the MAX7219 modules with the resistors or not ...  

## Wire connections

as seen from Wemos D1:

- D0 <- R560 <- RTC SQ // 1 HZ pulse in
- D1 -> R560 -> RTC SCL // I2C Clock
- D2 -> R560 -> RTC SDA // I2C Data
- D3 -> R560 -> Push Button -> GND
- D4 (internal LED)
- D5 -> R560 -> first Matrix CLK
- D6 (MISO, back channel Matrix, not used)
- D7 -> R560 -> first Matrix DI
- D8 -> R560 -> first Matrix CS
- GND -> Matrix GND, RTC GND
- 5V -> Matrix VCC, RTC VCC

The matrix modules are wired in series. The VCC and GND lines effectively run parallel through all modules, while each module's last DOUT / CS / CLK pin gets connected to the next module's first DIN / CS / CLK pin.

**NOTE: Be aware that full brightness of a single 4-blocks red LED board can exceed 1 A consumption (most of which will be converted to useless heat since it's overdriving the LEDs) so a test run of all 3 will overwhelm the usual 5V / 2A chargers.**

I do recommend (if you have the patience and soldering skill for fiddling with SMD components) to replace the default 10kΩ current setting resistors of the MAX7219 modules with 40-68 kΩ ones that will significantly lower both heat and power demand of the LED display ...

![Replaced resistor](/docs/img/2019-09-29_181650_IMG_web.jpg)