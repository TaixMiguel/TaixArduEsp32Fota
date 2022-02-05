/*
   esp32 firmware OTA
   Date: December 2018
   Author: Chris Joyce <https://github.com/chrisjoyce911/esp32FOTA/esp32FOTA>
   Purpose: Perform an OTA update from a bin located on a webserver (HTTP Only)
*/

#include "esp32fota.h"

esp32FOTA::esp32FOTA(String firmwareType, int firmwareVersion) {
	_firmwareVersion = firmwareVersion;
	_firmwareType = firmwareType;
}

// OTA Logic
void esp32FOTA::execOTA() {
	int contentLength = 0;
	bool isValidContentType = false;

	HTTPClient http;
	WiFiClientSecure client;
	//http.setConnectTimeout( 1000 );

	log_i("Connecting to: %s\r\n", _firmwareUrl.c_str());
	if (_firmwareUrl.substring(0, 5) == "https") {
		// We're downloading from a secure URL, but we don't want to validate the root cert.
		client.setInsecure();
		http.begin(client, _firmwareUrl);
	} else http.begin(_firmwareUrl);

	const char* headers[] = { "Content-Length", "Content-type" };
	http.collectHeaders(headers, 2);
	addHeadersParams(http);
	int httpCode = http.GET();

	if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
		contentLength = http.header("Content-Length").toInt();
		String contentType = http.header("Content-type");
		if (contentType == "application/octet-stream")
			isValidContentType = true;
	} else
		log_i("Connection to %s failed. Please check your setup", _firmwareUrl);

	// Check what is the contentLength and if content type is `application/octet-stream`
	log_i("contentLength : %i, isValidContentType : %s", contentLength, String(isValidContentType));

	// check contentLength and content type
	if (contentLength && isValidContentType) {
		WiFiClient& client = http.getStream();

		// Check if there is enough to OTA Update
		bool canBegin = Update.begin(contentLength);

		// If yes, begin
		if (canBegin) {
			Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
			// No activity would appear on the Serial monitor
			// So be patient. This may take 2 - 5mins to complete
			size_t written = Update.writeStream(client);

			if (written == contentLength) {
				Serial.println("Written : " + String(written) + " successfully");
			} else {
				Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
				// retry??
				// execOTA();
			}

			if (Update.end()) {
				Serial.println("OTA done!");
				if (Update.isFinished()) {
					Serial.println("Update successfully completed. Rebooting.");
					http.end();
					ESP.restart();
				} else {
					Serial.println("Update not finished? Something went wrong!");
				}
			} else {
				Serial.println("Error Occurred. Error #: " + String(Update.getError()));
			}
		} else {
			// not enough space to begin OTA
			// Understand the partitions and
			// space availability
			Serial.println("Not enough space to begin OTA");
			http.end();
		}
	} else {
		log_e("There was no content in the response");
		http.end();
	}
}

bool esp32FOTA::execHTTPcheck(String checkURL) {
	log_i("Getting HTTP: %s", checkURL.c_str());
	log_i("------");
	
	if ((WiFi.status() != WL_CONNECTED)) {
		log_w("WiFi not connected - skipping HTTP check");
		return false;  // WiFi not connected
	}

	HTTPClient http;
	WiFiClientSecure client;

	if (checkURL.substring(0, 5) == "https") {
		// We're downloading from a secure port, but we don't want to validate the root cert.
		client.setInsecure();
		http.begin(client, checkURL);
	} else http.begin(checkURL);
	addHeadersParams(http);

	int httpCode = http.GET(); //Make the request
	if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {  //Check is a file was returned
		String payload = http.getString();
		int str_len = payload.length() + 1;
		char JSONMessage[str_len];
		payload.toCharArray(JSONMessage, str_len);

		DynamicJsonDocument JSONResult(2048);
		DeserializationError err = deserializeJson(JSONResult, JSONMessage);
		http.end();  // We're done with HTTP - free the resources

		if (err) {  //Check for errors in parsing
			log_e("Parsing failed");
			return false;
		}

		if (JSONResult.is<JsonArray>()) {
			// We already received an array of multiple firmware types
			JsonArray arr = JSONResult.as<JsonArray>();
			for (JsonVariant JSONDocument : arr)
				if (checkJSONManifest(JSONDocument))
					return true;
		} else if (JSONResult.is<JsonObject>()) {
			if (checkJSONManifest(JSONResult.as<JsonVariant>()))
				return true;
		}

		return false; // We didn't get a hit against the above, return false
	} else {
		log_e("Error on HTTP request");
		http.end();
		return false;
	}
}

bool esp32FOTA::checkJSONManifest(JsonVariant JSONDocument) {
	if (strcmp(JSONDocument["type"].as<const char *>(), _firmwareType.c_str()) != 0) {
		log_i("Payload type in manifest %s doesn't match current firmware %s", JSONDocument["type"].as<const char *>(), _firmwareType.c_str() );
		log_i("Doesn't match type: %s", _firmwareType.c_str() );
		return false;  // Move to the next entry in the manifest
	}
	log_i("Payload type in manifest %s matches current firmware %s", JSONDocument["type"].as<const char *>(), _firmwareType.c_str() );

	if (JSONDocument["version"].is<uint16_t>()) {
		log_i("JSON version: %d (int)", JSONDocument["version"].as<uint16_t>());
		_newVersion = JSONDocument["version"].as<uint16_t>();
	} else {
		log_e("Invalid semver format received in manifest. Defaulting to 0");
		_newVersion = 0;
	}
	log_i("Payload firmware version: %d", _newVersion );

	if (JSONDocument["url"].is<String>()) {
		// We were provided a complete URL in the JSON manifest - use it
		_firmwareUrl = JSONDocument["url"].as<String>();
		if (JSONDocument["host"].is<String>())  // If the manifest provides both, warn the user
			log_w("Manifest provides both url and host - Using URL");
	} else if (JSONDocument["host"].is<String>() && JSONDocument["port"].is<uint16_t>() && JSONDocument["bin"].is<String>()) {
		// We were provided host/port/bin format - Build the URL
		if( JSONDocument["port"].as<uint16_t>() == 443 || JSONDocument["port"].as<uint16_t>() == 4433)
			_firmwareUrl = String("https://");
		else
			_firmwareUrl = String("http://");
		_firmwareUrl += JSONDocument["host"].as<String>() + ":" + String(JSONDocument["port"].as<uint16_t>()) + JSONDocument["bin"].as<String>();
	} else {
		// JSON was malformed - no firmware target was provided
		log_e("JSON manifest was missing both 'url' and 'host'/'port'/'bin' keys");
		return false;
	}

	if (_firmwareVersion < _newVersion) return true;
	return false;
}

int esp32FOTA::getVersionAvailable() {
	return _newVersion;
}

void esp32FOTA::addHeadersParams(HTTPClient& http) {
	if (swGitHub) {
		addHeaderParam(http, F("Accept"), "application/vnd.github.v3.raw");
		//addHeaderParam(http, F("User-Agent"), "request");
	}
	if (!token.isEmpty())
		addHeaderParam(http, F("authorization"), "Bearer " + token);
}
void esp32FOTA::addHeaderParam(HTTPClient& http, String name, String value) {
	http.addHeader(name, value);
	log_i("Added to header [%s]: %s", name.c_str(), value.c_str());
}
void esp32FOTA::setGitHub(String token) {
	this->token = token;
	swGitHub = true;
}