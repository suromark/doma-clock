# DoMa-Clock

The Dot Matrix Clock thing by suromark. It's a countdown clock, a regular clock, and a text scroller. Uses MAX7219 8x8 LED displays driven by an ESP8266 microcontroller to do its thing. It uses PlatformIO and various libraries (check their license info).

It's also most likely a hodgepodge of coding sins, but hey ... everyone has to start somewhere.

NOTE: The index.htm page of the built-in config web server is stored as SPIFFS file, so please do not just flash the program binary but also run PlatformIO's "Upload File System image" to make sure the Flash memory of the ESP is properly initialized.