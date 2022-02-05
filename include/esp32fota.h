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
    void setGitHub(String token);
    int getVersionAvailable();
    void execOTA();

  private:
    String _firmwareType, _firmwareUrl, token;
    int _firmwareVersion, _newVersion;
    bool swGitHub;

    void addHeadersParams(HTTPClient& http);
    bool checkJSONManifest(JsonVariant JSONDocument);
    void addHeaderParam(HTTPClient& http, String name, String value);
};

#endif
