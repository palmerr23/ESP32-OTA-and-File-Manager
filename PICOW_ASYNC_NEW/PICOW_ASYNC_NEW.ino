/*
  adapted from 
  esp32-asyncwebserver-fileupload-example

  https://github.com/smford/esp32-asyncwebserver-fileupload-example/tree/master/example-02
*/
#include <WiFi.h>
#define _RP2040W_AWS_LOGLEVEL_     1
#include <pico/cyw43_arch.h>
#include <AsyncWebServer_RP2040W.h>
#include <LEAmDNS.h>
//#include <FILESYS.h>
#include "LittleFS.h"
#define FILESYS LittleFS
char fsName[] = "LittleFS";
bool EEfound = false;
#include "webpages.h"

const char* host = "PICO";
const String default_ssid = "mySSID";
const String default_wifipassword = "MyPASS";
const String default_httpuser = "admin";
const String default_httppassword = "admin";
const int default_webserverporthttp = 80;

// configuration structure
struct Config {
  String ssid;               // wifi ssid
  String wifipassword;       // wifi password
  String httpuser;           // username to access web admin
  String httppassword;       // password to access web admin
  int webserverporthttp;     // http port number for web admin
};

// variables
Config config;                        // configuration
bool shouldReboot = false;            // schedule a reboot
AsyncWebServer *server;               // initialise webserver

// function defaults
String listFiles(bool ishtml = false);

void setup() {
  Serial.begin(115200);
  while(!Serial)
    delay(10);

  Serial.println("Booting ...");

  Serial.println("Mounting FILESYS ...");
  if (!FILESYS.begin()) {
    Serial.println("ERROR: Cannot mount FILESYS, Rebooting");
    rebootPico("ERROR: Cannot mount FILESYS, Rebooting");
  }

  Serial.println(listFiles());

  Serial.println("Loading Configuration ...");

  config.ssid = default_ssid;
  config.wifipassword = default_wifipassword;
  config.httpuser = default_httpuser;
  config.httppassword = default_httppassword;
  config.webserverporthttp = default_webserverporthttp;

  Serial.print("\nConnecting to Wifi: ");
  WiFi.begin(config.ssid.c_str(), config.wifipassword.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n\nNetwork Configuration:");
  Serial.println("----------------------");
  Serial.print("         SSID: "); Serial.println(WiFi.SSID());
  Serial.print("  Wifi Status: "); Serial.println(WiFi.status());
  Serial.print("Wifi Strength: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
  Serial.print("          MAC: "); Serial.println(WiFi.macAddress());
  Serial.print("           IP: "); Serial.println(WiFi.localIP());
  Serial.print("       Subnet: "); Serial.println(WiFi.subnetMask());
  Serial.print("      Gateway: "); Serial.println(WiFi.gatewayIP());
  Serial.print("        DNS 1: "); Serial.println(WiFi.dnsIP(0));
  Serial.print("        DNS 2: "); Serial.println(WiFi.dnsIP(1));
  Serial.print("        DNS 3: "); Serial.println(WiFi.dnsIP(2));
  Serial.println();

   if (!MDNS.begin(host))  
    Serial.println("Error setting up MDNS responder!");   
  else
    Serial.printf("mDNS responder started. Hotstname = http://%s.local\n", host);

  // configure web server
  Serial.println("Configuring Webserver ...");
  server = new AsyncWebServer(config.webserverporthttp);
  configureWebServer();

  // startup web server
  Serial.println("Starting Webserver ...");
  server->begin();
}

void loop() {
  // reboot if we've told it to reboot
  if (shouldReboot) {
    rebootPico("Web Admin Initiated Reboot");
  }
}

void rebootPico(String message) {
  Serial.print("Rebooting Pico W: "); Serial.println(message);
  delay(5000);
  rp2040.reboot();
}

// list all of the files, if ishtml=true, return html rather than simple text
String listFiles(bool ishtml) {
  FSInfo fs_info;
  String returnText = "";
  Serial.println("Listing files stored on FILESYS");
  File root = FILESYS.open("/", "r");
  File foundfile = root.openNextFile();
  if (ishtml) {
    returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th><th></th><th></th></tr>";
  }
  while (foundfile) {
    if (ishtml) {
      returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'download\')\">Download</button>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'delete\')\">Delete</button></tr>";
    } else {
      returnText += "File: " + String(foundfile.name()) + " Size: " + humanReadableSize(foundfile.size()) + "\n";
    }
    foundfile = root.openNextFile();
  }
  if (ishtml) {
    returnText += "</table>";
  }
  root.close();
  foundfile.close();
  FILESYS.info(fs_info);
  returnText += "<BR>" + humanReadableSize(fs_info.totalBytes) + " file system<BR>";
  returnText += humanReadableSize(fs_info.usedBytes) + " used, including directory structure (" + humanReadableSize(fs_info.blockSize) + " block size)<BR>";
  return returnText;
}

// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String humanReadableSize(const size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}
