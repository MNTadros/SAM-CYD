//main.ino

#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <TFT_eSPI.h>    // Graphics
#include <ArduinoJson.h> // JSON parsing
#include <FS.h>
#include <SPIFFS.h>

// —— Display settings ——
#define BG_COLOR TFT_BLACK
#define TXT_COLOR TFT_WHITE
#define LEFT_MARGIN 10 // px

// —— Update intervals ——
const uint32_t LOCAL_UPDATE_MS = 1000UL;
const uint32_t RESYNC_INTERVAL_MS = 3600000UL;
const uint32_t MESSAGE_REFRESH_MS = 5000UL;

// —— Device Config ——
String DEVICE_ID;
String WIFI_SSID;
String WIFI_PASS;
String API_BASE;
const char *TIME_URL = "http://worldtimeapi.org/api/timezone/America/Los_Angeles";

// —— TFT object ——
TFT_eSPI tft = TFT_eSPI();

// —— Timekeeping vars ——
uint32_t lastSyncLocalEpoch = 0;
uint32_t lastSyncMillis = 0;
uint32_t nextLocalUpdate = 0;
uint32_t nextResyncMillis = 0;
uint32_t lastMessageCheck = 0;

String lastMessageDisplayed = "";

uint32_t fetchLocalEpoch()
{
  HTTPClient http;
  http.begin(TIME_URL);
  int code = http.GET();
  if (code != HTTP_CODE_OK)
  {
    http.end();
    return 0;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err)
    return 0;

  uint32_t unixtime = doc["unixtime"] | 0;
  int32_t rawOffset = doc["raw_offset"] | 0;
  int32_t dstOffset = doc["dst_offset"] | 0;
  return unixtime + rawOffset + dstOffset;
}

String fetchMessageFromServer()
{
  HTTPClient http;
  String url = String(API_BASE) + DEVICE_ID;
  http.begin(url);

  int code = http.GET();
  Serial.println("HTTP GET code: " + String(code));
  if (code != HTTP_CODE_OK)
  {
    http.end();
    return "";
  }

  String payload = http.getString();
  Serial.println("Raw payload: " + payload);
  http.end();

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err)
  {
    Serial.print("JSON parse error: ");
    Serial.println(err.c_str());
    return "";
  }

  String sender = doc["sender"] | "Unknown";
  String message = doc["message"] | "No message";
  return sender + ": " + message;
}

String formatTime(uint32_t localEpoch)
{
  uint32_t seconds = localEpoch % 86400;
  uint8_t h = seconds / 3600;
  uint8_t m = (seconds % 3600) / 60;
  uint8_t s = seconds % 60;
  char buf[9];
  snprintf(buf, sizeof(buf), "%02u:%02u:%02u", h, m, s);
  return String(buf);
}

void displayTime(const String &ts)
{
  tft.setTextColor(TXT_COLOR, BG_COLOR);
  tft.setTextFont(4);
  tft.setTextSize(1);

  int16_t txtW = tft.textWidth(ts);
  int16_t txtH = tft.fontHeight();
  int16_t x = (tft.width() - txtW) / 2;
  int16_t y = tft.height() - txtH - LEFT_MARGIN;

  tft.fillRect(x, y, txtW, txtH, BG_COLOR);
  tft.setCursor(x, y);
  tft.print(ts);
}

void displayMessage(const String &msg)
{
  if (msg == lastMessageDisplayed)
    return; // don't redraw if same
  lastMessageDisplayed = msg;

  tft.setTextColor(TXT_COLOR, BG_COLOR);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.fillRect(0, 0, tft.width(), tft.height() - 40, BG_COLOR); // clear top part
  tft.setCursor(LEFT_MARGIN, 20);

  int maxWidth = tft.width() - 2 * LEFT_MARGIN;
  String line = "";
  for (int i = 0; i < msg.length(); i++)
  {
    line += msg[i];
    if (tft.textWidth(line) > maxWidth || msg[i] == '\n')
    {
      tft.println(line);
      line = "";
    }
  }
  if (line.length() > 0)
    tft.println(line);
}

void loadConfig()
{

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  fs::File file = SPIFFS.open("/config.txt", "r");

  while (file.available())
  {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.startsWith("device_id="))
    {
      DEVICE_ID = line.substring(strlen("device_id="));
    }
    else if (line.startsWith("ssid="))
    {
      WIFI_SSID = line.substring(strlen("ssid="));
    }
    else if (line.startsWith("password="))
    {
      WIFI_PASS = line.substring(strlen("password="));
    }
    else if (line.startsWith("api="))
    {
      API_BASE = line.substring(strlen("api="));
    }
  }

  file.close();
}

void setup()
{
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BG_COLOR);

  loadConfig();
  WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
  }

  uint32_t epoch;
  do
  {
    epoch = fetchLocalEpoch();
    if (epoch == 0)
      delay(1000);
  } while (epoch == 0);

  lastSyncLocalEpoch = epoch;
  lastSyncMillis = millis();
  nextLocalUpdate = lastSyncMillis + LOCAL_UPDATE_MS;
  nextResyncMillis = lastSyncMillis + RESYNC_INTERVAL_MS;

  displayTime(formatTime(lastSyncLocalEpoch));
}

void loop()
{
  uint32_t now = millis();

  // Update local clock every second
  if (now >= nextLocalUpdate)
  {
    uint32_t elapsedSec = (now - lastSyncMillis) / 1000;
    lastSyncLocalEpoch += elapsedSec;
    lastSyncMillis += elapsedSec * 1000;
    nextLocalUpdate = lastSyncMillis + LOCAL_UPDATE_MS;
    displayTime(formatTime(lastSyncLocalEpoch));
  }

  // Resync NTP hourly
  if (now >= nextResyncMillis)
  {
    uint32_t fresh = fetchLocalEpoch();
    if (fresh != 0)
    {
      lastSyncLocalEpoch = fresh;
      lastSyncMillis = now;
      nextLocalUpdate = now + LOCAL_UPDATE_MS;
      nextResyncMillis = now + RESYNC_INTERVAL_MS;
      displayTime(formatTime(lastSyncLocalEpoch));
    }
    else
    {
      nextResyncMillis = now + 60000;
    }
  }

  // Check message every 5 seconds
  if (now - lastMessageCheck > MESSAGE_REFRESH_MS)
  {
    lastMessageCheck = now;
    Serial.println("Fetching message from server...");
    String message = fetchMessageFromServer();
    if (message.length() > 0)
    {
      Serial.println("Received message: " + message);
      displayMessage(message);
    }
    else
    {
      Serial.println("No message or failed to fetch.");
    }
  }
}
