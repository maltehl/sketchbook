#include "bplaced.h"
#include <StreamString.h>


/**
 * return error code as int
 * @return int error code
 */
int HTTPDatagetLastError(void)
{
    return _lastError;
}

PostResult handleData(HTTPClient& http, const String& currentVersion)
{

    HTTPDataResult ret = HTTP_DATA_FAILED;

    // use HTTP/1.0 for update since the update handler not support any transfer Encoding
    http.useHTTP10(true);
    http.setTimeout(8000);
    http.setUserAgent(F("ESP8266-http-Data-Transfer"));
    http.addHeader(F("x-ESP8266-STA-MAC"), WiFi.macAddress());
    http.addHeader(F("x-ESP8266-AP-MAC"), WiFi.softAPmacAddress());
    http.addHeader(F("x-ESP8266-free-space"), String(ESP.getFreeSketchSpace()));
    http.addHeader(F("x-ESP8266-sketch-size"), String(ESP.getSketchSize()));
    http.addHeader(F("x-ESP8266-chip-size"), String(ESP.getFlashChipRealSize()));
    http.addHeader(F("x-ESP8266-sdk-version"), ESP.getSdkVersion());

    if(currentVersion && currentVersion[0] != 0x00) {
        http.addHeader(F("x-ESP8266-version"), currentVersion);
    }

    const char * headerkeys[] = { "x-MD5" };
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);

    // track these headers
    http.collectHeaders(headerkeys, headerkeyssize);


    int code = http.GET();
    int len = http.getSize();

    if(code <= 0) {
        DEBUG_HTTP_UPDATE("[httpUpdate] HTTP error: %s\n", http.errorToString(code).c_str());
        _lastError = code;
        http.end();
        return HTTP_UPDATE_FAILED;
    }


    DEBUG_HTTP_UPDATE("[httpUpdate] Header read fin.\n");
    DEBUG_HTTP_UPDATE("[httpUpdate] Server header:\n");
    DEBUG_HTTP_UPDATE("[httpUpdate]  - code: %d\n", code);
    DEBUG_HTTP_UPDATE("[httpUpdate]  - len: %d\n", len);

    if(http.hasHeader("x-MD5")) {
        DEBUG_HTTP_UPDATE("[httpUpdate]  - MD5: %s\n", http.header("x-MD5").c_str());
    }

    DEBUG_HTTP_UPDATE("[httpUpdate] ESP8266 info:\n");
    DEBUG_HTTP_UPDATE("[httpUpdate]  - free Space: %d\n", ESP.getFreeSketchSpace());
    DEBUG_HTTP_UPDATE("[httpUpdate]  - current Sketch Size: %d\n", ESP.getSketchSize());

    if(currentVersion && currentVersion[0] != 0x00) {
        DEBUG_HTTP_UPDATE("[httpUpdate]  - current version: %s\n", currentVersion);
    }

    switch(code) {
    case HTTP_CODE_OK:  ///< OK (Start Update)
        if(len > 0) {
            bool startUpdate = true;
            if(spiffs) {
                size_t spiffsSize = ((size_t) &_SPIFFS_end - (size_t) &_SPIFFS_start);
                if(len > (int) spiffsSize) {
                    DEBUG_HTTP_UPDATE("[httpUpdate] spiffsSize to low (%d) needed: %d\n", spiffsSize, len);
                    startUpdate = false;
                }
            } else {
                if(len > (int) ESP.getFreeSketchSpace()) {
                    DEBUG_HTTP_UPDATE("[httpUpdate] FreeSketchSpace to low (%d) needed: %d\n", ESP.getFreeSketchSpace(), len);
                    startUpdate = false;
                }
            }

            if(!startUpdate) {
                _lastError = HTTP_UE_TOO_LESS_SPACE;
                ret = HTTP_UPDATE_FAILED;
            } else {

                WiFiClient * tcp = http.getStreamPtr();

                WiFiUDP::stopAll();
                WiFiClient::stopAllExcept(tcp);

                delay(100);

                int command;

                if(spiffs) {
                    command = U_SPIFFS;
                    DEBUG_HTTP_UPDATE("[httpUpdate] runUpdate spiffs...\n");
                } else {
                    command = U_FLASH;
                    DEBUG_HTTP_UPDATE("[httpUpdate] runUpdate flash...\n");
                }

                if(!spiffs) {
                    uint8_t buf[4];
                    if(tcp->peekBytes(&buf[0], 4) != 4) {
                        DEBUG_HTTP_UPDATE("[httpUpdate] peekBytes magic header failed\n");
                        _lastError = HTTP_UE_BIN_VERIFY_HEADER_FAILED;
                        http.end();
                        return HTTP_UPDATE_FAILED;
                    }

                    // check for valid first magic byte
                    if(buf[0] != 0xE9) {
                        DEBUG_HTTP_UPDATE("[httpUpdate] magic header not starts with 0xE9\n");
                        _lastError = HTTP_UE_BIN_VERIFY_HEADER_FAILED;
                        http.end();
                        return HTTP_UPDATE_FAILED;

                    }

                    uint32_t bin_flash_size = ESP.magicFlashChipSize((buf[3] & 0xf0) >> 4);

                    // check if new bin fits to SPI flash
                    if(bin_flash_size > ESP.getFlashChipRealSize()) {
                        DEBUG_HTTP_UPDATE("[httpUpdate] magic header, new bin not fits SPI Flash\n");
                        _lastError = HTTP_UE_BIN_FOR_WRONG_FLASH;
                        http.end();
                        return HTTP_UPDATE_FAILED;
                    }
                }

                if(runUpdate(*tcp, len, http.header("x-MD5"), command)) {
                    ret = HTTP_UPDATE_OK;
                    DEBUG_HTTP_UPDATE("[httpUpdate] Update ok\n");
                    http.end();

                    if(_rebootOnUpdate) {
                        ESP.restart();
                    }

                } else {
                    ret = HTTP_UPDATE_FAILED;
                    DEBUG_HTTP_UPDATE("[httpUpdate] Update failed\n");
                }
            }
        } else {
            _lastError = HTTP_UE_SERVER_NOT_REPORT_SIZE;
            ret = HTTP_UPDATE_FAILED;
            DEBUG_HTTP_UPDATE("[httpUpdate] Content-Length is 0 or not set by Server?!\n");
        }
        break;
    case HTTP_CODE_NOT_MODIFIED:
        ///< Not Modified (No updates)
        ret = HTTP_UPDATE_NO_UPDATES;
        break;
    case HTTP_CODE_NOT_FOUND:
        _lastError = HTTP_UE_SERVER_FILE_NOT_FOUND;
        ret = HTTP_UPDATE_FAILED;
        break;
    case HTTP_CODE_FORBIDDEN:
        _lastError = HTTP_UE_SERVER_FORBIDDEN;
        ret = HTTP_UPDATE_FAILED;
        break;
    default:
        _lastError = HTTP_UE_SERVER_WRONG_HTTP_CODE;
        ret = HTTP_UPDATE_FAILED;
        DEBUG_HTTP_UPDATE("[httpUpdate] HTTP Code is (%d)\n", code);
        //http.writeToStream(&Serial1);
        break;
    }

    http.end();
    return ret;
}
