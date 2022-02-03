/*
   esp32 firmware OTA
   Date: December 2018
   Author: Chris Joyce <https://github.com/chrisjoyce911/esp32FOTA/esp32FOTA>
   Purpose: Perform an OTA update from a bin located on a webserver (HTTP Only)
*/

#ifndef esp32fota_h
#define esp32fota_h

#include "Arduino.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

class esp32FOTA {
  public:
    esp32FOTA(String firwmareType, int firwmareVersion);
    bool execHTTPcheck(String checkURL);
    int getVersionAvailable();
    void execOTA();

  private:
    String _firmwareType, _firmwareUrl;
    int _firmwareVersion, _newVersion;

    bool checkJSONManifest(JsonVariant JSONDocument);
};

#endif
