/**
 * Based on Arduino Real-Time Slack Bot, Copyright (C) 2016, Uri Shaked.
 * Licensed under the MIT License
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include "config.h"

#define USE_SERIAL Serial

#define RELAY_PIN 12
#define RELAY_WAIT 1000

#define SLACK_API_SSL_CERT                                               \
    "-----BEGIN CERTIFICATE-----\n"                                      \
    "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
    "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
    "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
    "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
    "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
    "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
    "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
    "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
    "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
    "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
    "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
    "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
    "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
    "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
    "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
    "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n"                 \
    "-----END CERTIFICATE-----\n"
#define SLACK_API_SSL_CERT_FINGERPRINT "C1 0D 53 49 D2 3E E5 2B A2 61 D5 9E 6F 99 0D 3D FD 8B B2 B3"
#define NOPE_RESPONSES 4

WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
String nope_responses[NOPE_RESPONSES] = {"https://i.imgur.com/JdiC5zK.gif", "https://i.gifer.com/7GTC.gif",
                                         "https://i.imgur.com/Wt4gkK0.gif", "https://media.giphy.com/media/PAO4KoQ532CRi/giphy.gif"};

long nextCmdId = 1;
bool connected = false;
unsigned long lastPing = 0;

void openDoors() {
    USE_SERIAL.printf("Opening doors...");
    digitalWrite(RELAY_PIN, HIGH);
    delay(RELAY_WAIT);
    USE_SERIAL.printf("done\n");
    digitalWrite(RELAY_PIN, LOW);
}

void sendPing() {
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["type"] = "ping";
    root["id"] = nextCmdId++;
    String json;
    root.printTo(json);
    webSocket.sendTXT(json);
}

void processSlackMessage(char *payload) {
    boolean respond = true;
    String json;
    DynamicJsonBuffer jsonBuffer;
    DynamicJsonBuffer jsonRespBuffer;
    long rand;

    JsonObject &root = jsonBuffer.parse(payload);
    if (!root.success()) {
        USE_SERIAL.println("Parsing failed!");
        return;
    }

    if (!root.containsKey("type")) {
        return;
    }

    if (root["type"] == F("message")) {
        JsonObject &resp = jsonRespBuffer.createObject();
        resp["type"] = "message";
        resp["id"] = nextCmdId++;
        resp["channel"] = root["channel"];
        respond = true;
        if (root["channel"] == F(SLACK_API_CHANNEL)) {
            String message = root["text"];
            message.toLowerCase();
            if (message.indexOf("open") > -1) {
                openDoors();
                resp["text"] = "Scruffy's work here is done.";
            } else {
                respond = false;
            }
        } else {
            if (root.containsKey("subtype") && root["subtype"] == F("bot_message")) {
                respond = false;
            } else {
                rand = random(NOPE_RESPONSES);
                USE_SERIAL.printf("rand = %i\n", rand);
                USE_SERIAL.println(nope_responses[rand]);
                resp["text"] = nope_responses[rand];
            }
        }

        if (respond) {
            resp.printTo(json);
            USE_SERIAL.printf("[WebSocker] Sending response: %s\n", json.c_str());
            webSocket.sendTXT(json);
        }
    }
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t len) {
    switch (type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[WebSocket] Disconnected :-( \n");
            connected = false;
            break;

        case WStype_CONNECTED:
            USE_SERIAL.printf("[WebSocket] Connected to: %s\n", payload);
            sendPing();
            break;

        case WStype_TEXT:
            USE_SERIAL.printf("[WebSocket] Message: %s\n", payload);
            processSlackMessage((char *)payload);
            break;
    }
}

bool connectToSlack() {
    // Step 1: Find WebSocket address via RTM API (https://api.slack.com/methods/rtm.connect)
    HTTPClient http;
    http.begin("https://slack.com/api/rtm.connect?no_latest=1&token=" SLACK_API_TOKEN, SLACK_API_SSL_CERT);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        USE_SERIAL.printf("HTTP GET failed with code %d\n", httpCode);
        return false;
    }

    WiFiClient *client = http.getStreamPtr();
    client->find("wss:\\/\\/");
    String host = client->readStringUntil('\\');
    String path = client->readStringUntil('"');
    path.replace("\\/", "/");

    // Step 2: Open WebSocket connection and register event handler
    USE_SERIAL.println("WebSocket Host=" + host + " Path=" + path);
    webSocket.beginSSL(host, 443, path, SLACK_API_SSL_CERT_FINGERPRINT, "");
    webSocket.onEvent(webSocketEvent);
    return true;
}

void setup() {
    USE_SERIAL.begin(115200);
    USE_SERIAL.setDebugOutput(true);

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);
    while (WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
    }

    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

void loop() {
    webSocket.loop();

    if (connected) {
        // Send ping every 5 seconds, to keep the connection alive
        if (millis() - lastPing > 5000) {
            sendPing();
            lastPing = millis();
        }
    } else {
        // Try to connect / reconnect to slack
        connected = connectToSlack();
        if (!connected) {
            delay(500);
        }
    }
}
