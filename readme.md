ESP32 OTA and File System Management

The Arduino V2 IE does not yet include the Sketch File Upload plugin.

This program provides a web-based interface for
OTA
File system formatting
File uploding
FS directory listing
File editing
File deletion

It may be accessed via from a browser by its IP address or via http://ESP32OTA.local


Limitations:
It must be compiled for  a specific (SPIFFS or LittleFS) file system.
It does not support LittleFS folder hierarchies.
WiFi credentials must be compiled in.
The code has only been tested in a Chrome desktop browser on Windows. While it should function correclty in Mozilla-style browsers across all desktop OS platforms, functionality on mobile and other desktop browsers is untested.