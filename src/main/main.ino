#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <TFT_eSPI.h>      // Graphics
#include <ArduinoJson.h>   // JSON parsing

// —— Display settings ——
#define BG_COLOR     TFT_BLACK
#define TXT_COLOR    TFT_WHITE
#define LEFT_MARGIN  10    // px

// —— Update intervals ——
const uint32_t LOCAL_UPDATE_MS    = 1000UL;    // tick every second
const uint32_t RESYNC_INTERVAL_MS = 3600000UL; // resync every hour

// —— Wi-Fi credentials ——
const char* WIFI_SSID = "SSID"; // replace with your Wi-Fi SSID
const char* WIFI_PASS = "PASSWORD"; // replace with your Wi-Fi password

// —— Time API endpoint ——
const char* TIME_URL  = "http://worldtimeapi.org/api/timezone/America/Los_Angeles";

// —— TFT object ——
TFT_eSPI tft = TFT_eSPI();

// —— Timekeeping vars ——
uint32_t lastSyncLocalEpoch = 0; // seconds since local midnight at last sync
uint32_t lastSyncMillis     = 0; // millis() when we last synced
uint32_t nextLocalUpdate    = 0;
uint32_t nextResyncMillis   = 0;

// Fetch the current Unix epoch and timezone offset (raw + DST), return local epoch
uint32_t fetchLocalEpoch() {
  HTTPClient http;
  http.begin(TIME_URL);
  if (http.GET() != HTTP_CODE_OK) {
    http.end();
    return 0;
  }
  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, payload)) return 0;

  uint32_t unixtime   = doc["unixtime"]   | 0; // UTC epoch
  int32_t  rawOffset  = doc["raw_offset"] | 0; // e.g. -28800
  int32_t  dstOffset  = doc["dst_offset"] | 0; // e.g. 3600
  return unixtime + rawOffset + dstOffset;
}

// Format a local-epoch into "HH:MM:SS"
String formatTime(uint32_t localEpoch) {
  uint32_t secondsSinceMidnight = localEpoch % 86400;
  uint8_t  h = secondsSinceMidnight / 3600;
  uint8_t  m = (secondsSinceMidnight % 3600) / 60;
  uint8_t  s = secondsSinceMidnight % 60;
  char buf[9];
  snprintf(buf, sizeof(buf), "%02u:%02u:%02u", h, m, s);
  return String(buf);
}

// Draw the time centered at the bottom, clearing only that area
void displayTime(const String &ts) {
  tft.setTextColor(TXT_COLOR, BG_COLOR);
  tft.setTextFont(4);
  tft.setTextSize(1);

  int16_t txtW = tft.textWidth(ts);
  int16_t txtH = tft.fontHeight();
  int16_t x    = (tft.width()  - txtW) / 2;
  int16_t y    = tft.height() - txtH - LEFT_MARGIN;

  // clear only the clock rectangle
  tft.fillRect(x, y, txtW, txtH, BG_COLOR);
  tft.setCursor(x, y);
  tft.print(ts);
}

void setup() {
  Serial.begin(115200);

  // Init display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BG_COLOR);

  // Connect Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }

  // Initial network sync (block until successful)
  uint32_t epoch;
  do {
    epoch = fetchLocalEpoch();
    if (epoch == 0) delay(1000);
  } while (epoch == 0);

  lastSyncLocalEpoch = epoch;
  lastSyncMillis     = millis();
  nextLocalUpdate    = lastSyncMillis + LOCAL_UPDATE_MS;
  nextResyncMillis   = lastSyncMillis + RESYNC_INTERVAL_MS;

  // Draw the starting time
  displayTime(formatTime(lastSyncLocalEpoch));
}

void loop() {
  uint32_t now = millis();

  // Local tick every second
  if (now >= nextLocalUpdate) {
    uint32_t elapsedSec = (now - lastSyncMillis) / 1000;
    lastSyncLocalEpoch += elapsedSec;
    lastSyncMillis     += elapsedSec * 1000;
    nextLocalUpdate     = lastSyncMillis + LOCAL_UPDATE_MS;
    displayTime(formatTime(lastSyncLocalEpoch));
  }

  // Hourly resync to correct drift
  if (now >= nextResyncMillis) {
    uint32_t fresh = fetchLocalEpoch();
    if (fresh != 0) {
      lastSyncLocalEpoch = fresh;
      lastSyncMillis     = now;
      nextLocalUpdate    = now + LOCAL_UPDATE_MS;
      nextResyncMillis   = now + RESYNC_INTERVAL_MS;
      displayTime(formatTime(lastSyncLocalEpoch));
    } else {
      // retry sooner if failed
      nextResyncMillis = now + 60000;
    }
  }
}
