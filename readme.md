# ESP32 OTA and File System Management

The Arduino V2.0 IDE does not yet include the 'Sketch Data Upload' plugin.

## This program provides a web-based interface for
* OTA
* File system formatting
* FS directory listing
* File uploding
* File editing
* File deletion

It may be accessed via from a web browser by its IP address or via http://ESP32OTA.local

## Limitations:
* It must be compiled for either the SPIFFS or LittleFS file system. 
* It does not support LittleFS folder hierarchies.
* WiFi credentials must be compiled in.
* The code has only been tested in a Chrome desktop browser on Windows. While it should function correclty in Mozilla-style browsers across all desktop OS platforms, functionality on mobile and other desktop browsers is untested.
* Assumes ESP32 v2.x (ESP32-IDF v4.x) for Arduino 1.8.x or Arduino 2.0.x 
* It does not require additional libraries to be installed.
* Tested on ESP32-DOWD (WROOM and WROVER) and ESP32-S3 (WROOM).

## Using this code:
* Set your WiFi credentials in the myWiFi.h file
* Select a board partition scheme that allows SPIFFS (and OTA if you require it). Either the first option or fourth from the bottom are known to work. Third from the top and fifth from the bottom (large FS) should work on ESP32's with sufficient resources, but haven't been tested.

![Screenshot 2023-02-14 085739](https://user-images.githubusercontent.com/14856369/218584574-e9b7bc12-1cc5-4a47-a1dc-c944b132fa9f.png)
* Compile and load the program, using Arduino, onto your ESP32. 
* Access the http://ESP32OTA.local URL from your host machine.
* Perform file functions (format, load, edit, delete) and upload other programs (OTA) on the ESP32

## To Do
* Add VFS/FATFS support
