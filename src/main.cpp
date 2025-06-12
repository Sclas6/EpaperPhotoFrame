#include <Arduino.h>
#include <M5Unified.h>
#include <esp_wifi.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "epd7in3e.h"
#include "imagedata.h"
#include "wifi_secret.h"

#define WIFI_CONNECTION_TIMEOUT (10 * 1000)
#define HTTP_ENDPOINT "http://192.168.0.78:8000"

uint32_t count;
HTTPClient http;
Epd epd;
uint8_t httpBuff[1024 * 10] = {0};

String sensorReadings;

bool httpGetAndUpdateEpaper()
{
  char url[256] = {0};
  char userAgent[256] = {0};
  sprintf(url, "%s", HTTP_ENDPOINT);
  sprintf(userAgent, "%s %s %s", __FILE__, __DATE__, __TIME__);
  //
  Serial.printf("HTTP GET: %s\n", url);
  http.setUserAgent(userAgent);
  http.begin(url);
  //
  int code = http.GET();
  if (code != 200) {
    Serial.printf("HTTP Status: %d\r\n", code);
    return false;
  }
  //
  int len = http.getSize();
  if (len == 0) {
    Serial.printf("Content-Length: %d\r\n", len);
    return false;
  }
  // 1. データ送信開始
  epd.SendCommand(0x10);
  //
  WiFiClient *stream = http.getStreamPtr();
  int readSize = 0;
  while (http.connected() && (len > 0 || len == -1))
  {
    size_t size = stream->available();
    if (size) {
      int c = stream->readBytes(httpBuff, min(sizeof(httpBuff), size));

      for (int i = 0; i < c; i++) {
        // 2. 画素データ送信
        epd.SendData(httpBuff[i]);
        M5.Display.print(httpBuff[i]);
      }
      if (len > 0) {
        len -= c;
        readSize += c;
      }
      Serial.print(".");
    }
    delay(1);
  }
  Serial.println("");
  // 3. ディスプレイON
  epd.TurnOnDisplay();
  http.end();
  // 4. スリープ
  epd.Sleep();
  Serial.printf("httpGet(): Done. readSize=%d\n", readSize);
  return true;
}

void setup() {

    auto cfg = M5.config();

    M5.begin(cfg);
    USBSerial.begin(115200);
    //M5.Display.setTextSize(3);
    USBSerial.println("Hello World!!");
    count = 0;

    unsigned long timeoutTime = millis() + WIFI_CONNECTION_TIMEOUT;
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (millis() >= timeoutTime) {
            Serial.println("WiFi Connection Timeout");
            M5.Display.print("WiFi Connection Timeout");
            return;
        }
    }
    M5.Display.setCursor(0, 0);
    M5.Display.print(WiFi.localIP());
    epd.Init();
    M5.Display.setCursor(0, 10);
    httpGetAndUpdateEpaper();
    //epd.Init();

    //epd.Init();
    //epd.Clear(EPD_7IN3F_WHITE);
    //epd.EPD_7IN3F_Display(gImage_7in3f);
    epd.Sleep();

}

void loop() {

    delay(1000);

}