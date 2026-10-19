#include <Arduino.h>
#include <DHT.h>
#include <Arduino_GFX_Library.h>

// ---------------------------------------------------------------------------
// Pin map
// ---------------------------------------------------------------------------
#define LED_PIN 2 // onboard LED, heartbeat blink

#define DHT_PIN 27
#define DHT_TYPE DHT22

#define PIR_PIN 26

#define MQ135_PIN 34

#define BUZZER_PIN 25

#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 17
#define TFT_SCK 18
#define TFT_MOSI 23

// ---------------------------------------------------------------------------

DHT dht(DHT_PIN, DHT_TYPE);

Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

unsigned long lastDhtRead = 0;
const unsigned long DHT_INTERVAL = 2500;
float temperatureC = NAN;
float humidity = NAN;

unsigned long lastHeartbeat = 0;
bool ledState = false;

bool pirState = false;
bool lastPirState = false;

unsigned long lastMotionTime = 0;
const unsigned long SCREEN_ON_DURATION = 15000; // keeps screen on this long after last motion
bool screenBlanked = false;

unsigned long lastDraw = 0;
const unsigned long DRAW_INTERVAL = 500;

unsigned long lastMq135Log = 0;
const unsigned long MQ135_LOG_INTERVAL = 2000;

void drawScreen()
{
  gfx->fillScreen(BLACK);

  gfx->setTextColor(GREEN);
  gfx->setTextSize(2);
  gfx->setCursor(40, 30);
  gfx->println("ESP32 alive!");

  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);

  gfx->setCursor(40, 80);
  if (isnan(temperatureC))
  {
    gfx->println("Temp: --.-C");
  }
  else
  {
    gfx->print("Temp: ");
    gfx->print(temperatureC, 1);
    gfx->println("C");
  }

  gfx->setCursor(40, 110);
  if (isnan(humidity))
  {
    gfx->println("Hum:  --.-%");
  }
  else
  {
    gfx->print("Hum:  ");
    gfx->print(humidity, 1);
    gfx->println("%");
  }

  int mq135Raw = analogRead(MQ135_PIN);
  const char *airQuality;
  uint16_t airColor;
  if (mq135Raw < 1000)
  {
    airQuality = "Good";
    airColor = GREEN;
  }
  else if (mq135Raw < 2000)
  {
    airQuality = "Medium";
    airColor = YELLOW;
  }
  else
  {
    airQuality = "Bad";
    airColor = RED;
  }
  digitalWrite(BUZZER_PIN, mq135Raw >= 1200 ? HIGH : LOW);

  gfx->setTextColor(airColor);
  gfx->setCursor(40, 140);
  gfx->print("Air:  ");
  gfx->println(airQuality);

  gfx->setTextColor(pirState ? RED : WHITE);
  gfx->setCursor(40, 175);
  gfx->print("PIR: ");
  gfx->println(pirState ? "MOTION" : "clear");
}

void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println("ESP32 is alive!");

  pinMode(LED_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  dht.begin();

  gfx->begin();
  gfx->fillScreen(BLACK);
  gfx->setTextColor(GREEN);
  gfx->setTextSize(2);
  gfx->setCursor(40, 100);
  gfx->println("Booting...");
}

void loop()
{
  unsigned long now = millis();

  // Heartbeat blink, doubles as "still running" indicator
  if (now - lastHeartbeat >= 500)
  {
    lastHeartbeat = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    Serial.println("ESP32 is still alive!");
  }

  // DHT22
  if (now - lastDhtRead >= DHT_INTERVAL)
  {
    lastDhtRead = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t))
    {
      humidity = h;
      temperatureC = t;
      Serial.printf("DHT22 -> Temp: %.1f C  Hum: %.1f %%\n", t, h);
    }
    else
    {
      Serial.println("DHT22 -> read failed");
    }
  }

  // PIR
  pirState = digitalRead(PIR_PIN) == HIGH;
  if (pirState)
  {
    lastMotionTime = now;
  }
  if (pirState != lastPirState)
  {
    lastPirState = pirState;
    Serial.println(pirState ? "PIR -> motion detected" : "PIR -> motion ended");
  }

  // MQ135
  if (now - lastMq135Log >= MQ135_LOG_INTERVAL)
  {
    lastMq135Log = now;
    int raw = analogRead(MQ135_PIN);
    // x2 corrects for the 10k/10k voltage divider on AO, giving the sensor's actual output
    float voltage = raw * (3.3f / 4095.0f) * 2.0f;
    Serial.printf("MQ135 -> raw: %d  voltage: %.2f V\n", raw, voltage);
  }

  // TFT refresh — screen stays on for SCREEN_ON_DURATION after last motion
  bool screenActive = (now - lastMotionTime) < SCREEN_ON_DURATION;
  if (screenActive)
  {
    if (now - lastDraw >= DRAW_INTERVAL)
    {
      lastDraw = now;
      drawScreen();
      screenBlanked = false;
    }
  }
  else if (!screenBlanked)
  {
    gfx->fillScreen(BLACK);
    screenBlanked = true;
  }
}
