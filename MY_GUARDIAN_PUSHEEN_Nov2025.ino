#include <ESP8266WiFi.h>
#include "Mac.h"

extern "C" {
  #include "user_interface.h"
}

#include <Adafruit_NeoPixel.h>
#include <math.h>

// ===== SNIFFER SETTINGS ===== //
#define WIFI_CHANNEL       1
#define CHANNEL_HOPPING    true
#define MAX_CHANNEL        13
#define SCAN_TIME_MS       500

// ===== NEOPIXEL SETTINGS ===== //
#define NEO_PIN            5
#define NEO_COUNT          5
#define NEO_BRIGHTNESS     255   // max brightness

Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

// ===== Deep Orange Flicker Base ===== //
const uint8_t BASE_R = 255;
const uint8_t BASE_G = 80;
const uint8_t BASE_B = 0;

// Attack counters
unsigned long deauth = 0;
unsigned long dissoc = 0;

// Timing
unsigned long prevScanTime = 0;
unsigned long curTime      = 0;
int curChannel             = WIFI_CHANNEL;

unsigned long lastCandleUpdate = 0;
const unsigned long CANDLE_INTERVAL_MS = 30;

// ======== Perlin-like noise functions ======== //
static uint8_t hash2D(int x, int y) {
  uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u;
  h = h ^ (h >> 16);
  return (uint8_t)(h & 0xFF);
}

static float valueAt(int x, int y) {
  return hash2D(x, y) / 255.0f;
}

static float fade(float t) {
  return t * t * t * (t * (t * 6 - 15) + 10);
}

static float lerp(float a, float b, float t) {
  return a + t * (b - a);
}

float noise2D(float x, float y) {
  int x0 = (int)floorf(x);
  int y0 = (int)floorf(y);
  int x1 = x0 + 1;
  int y1 = y0 + 1;

  float fx = x - x0;
  float fy = y - y0;

  float v00 = valueAt(x0, y0);
  float v10 = valueAt(x1, y0);
  float v01 = valueAt(x0, y1);
  float v11 = valueAt(x1, y1);

  float sx = fade(fx);
  float sy = fade(fy);

  float i1 = lerp(v00, v10, sx);
  float i2 = lerp(v01, v11, sx);
  return lerp(i1, i2, sy);
}

// ======== Sniffer callback ======== //
void sniffer(uint8_t *buf, uint16_t len) {
  if (buf[12] == 0xA0) {          // Dissociation
    dissoc = 500;
  } else if (buf[12] == 0xC0) {   // Deauthentication
    deauth = 500;
  } else {
    if (deauth >= 1) deauth--;
    if (dissoc >= 1) dissoc--;
  }
}

// ======== LED behavior ======== //
void showAlertColor() {
  uint8_t r = 0, g = 0, b = 0;

  bool hasDeauth = (deauth > 0);
  bool hasDissoc = (dissoc > 0);

  if (hasDeauth && !hasDissoc) {
    // Deauth → Red
    r = 255; g = 0; b = 0;
  }
  else if (!hasDeauth && hasDissoc) {
    // Dissoc → Blue
    r = 0; g = 0; b = 255;
  }
  else if (hasDeauth && hasDissoc) {
    // Both → Purple
    r = 180; g = 0; b = 255;
  }

  for (int i = 0; i < NEO_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

void updateCandleFlicker() {
  unsigned long now = millis();
  if (now - lastCandleUpdate < CANDLE_INTERVAL_MS) return;
  lastCandleUpdate = now;

  float t = now * 0.0008f;

  for (int i = 0; i < NEO_COUNT; i++) {
    float x = i * 0.7f;
    float n = noise2D(x, t);

    float bfactor = 0.6f + n * 0.7f;
    if (bfactor < 0.4f) bfactor = 0.4f;
    if (bfactor > 1.3f) bfactor = 1.3f;

    uint8_t r = constrain(BASE_R * bfactor, 0, 255);
    uint8_t g = constrain(BASE_G * bfactor, 0, 255);
    uint8_t b = constrain(BASE_B * bfactor, 0, 255);

    int8_t gJitter = random(-4, 5);
    int8_t bJitter = random(-4, 5);
    g = constrain((int)g + gJitter, 0, 255);
    b = constrain((int)b + bJitter, 0, 255);

    strip.setPixelColor(i, strip.Color(r, g, b));
  }

  strip.show();
}

void updateLeds() {
  if (deauth > 0 || dissoc > 0) {
    showAlertColor();
  } else {
    updateCandleFlicker();
  }
}

// ======== SETUP ======== //
void setup() {
  Serial.begin(115200);

  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  WiFi.disconnect();
  wifi_set_promiscuous_rx_cb(sniffer);
  wifi_set_channel(curChannel);
  wifi_promiscuous_enable(1);

  strip.begin();
  strip.setBrightness(NEO_BRIGHTNESS);
  strip.show();

  Serial.println("starting!");
}

// ======== LOOP ======== //
void loop() {
  curTime = millis();
  updateLeds();

  if (curTime - prevScanTime >= SCAN_TIME_MS) {
    prevScanTime = curTime;

    if (CHANNEL_HOPPING) {
      curChannel++;
      if (curChannel > MAX_CHANNEL) curChannel = 1;
      wifi_set_channel(curChannel);
    }
  }
}
