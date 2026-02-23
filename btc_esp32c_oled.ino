#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <HTTPClient.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// Pin Definitions
#define SDA_PIN 5
#define SCL_PIN 6
#define BUTTON_PIN 9 // Define the button pin

// Display Configuration
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);   // EastRising 0.42" OLED

static const char *symbol= "BTCUSDT";
#define BINANCE_API "https://api.binance.com/api/v3/ticker/price?symbol=" // API call to binance

const char* ssid = "REDE TIM 2G";
const char* password = "12345678";

static float prices=0;
static float openPrices=0;

static uint32_t lastApiMs = 0; // Time of last api call

// Variables
int currentNetwork = 0;           // Track the currently displayed network
int totalNetworks = 0;            // Total number of networks found
unsigned long lastScanTime = 0;   // Last Wi-Fi scan time
const unsigned long scanInterval = 10000; // Refresh Wi-Fi scan every 10 seconds
bool firstBoot = true;            // Track whether it's the first boot

// Button Debounce Variables
bool lastButtonState = HIGH;      // Previous button state
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;  // Debounce delay in milliseconds

// Scrolling Variables
int ssidScrollPosition = 0;       // Scroll position for the current SSID
unsigned long lastScrollTime = 0; // Last time the SSID scrolled
const int scrollSpeed = 4;        // Speed of scrolling
const unsigned long scrollDelay = 50; // Delay between scroll updates in ms

void setup() {
  Serial.begin(115200); // Start serial monitor for debugging
  // Initialize I2C and OLED display
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();

  // Initialize Wi-Fi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // Ensure we start fresh

  // Initialize button
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Use internal pull-up resistor

  // Display intro
  displayIntro();
  delay(2000); // Show for 2 seconds

  // Debug message
  Serial.println("Setup complete.");
}

void loop() {
  fetchPrice();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);

  u8g2.setCursor(17, 10);
  u8g2.print(symbol);

  u8g2.setCursor(15, 22);
  u8g2.print(prices);
  u8g2.sendBuffer();
  delay(15000);
}

// Simple HTTP GET – returns body as String or empty on fail
static String httpGET(const char *url, uint32_t timeoutMs = 3000)
{
    HTTPClient http;
    http.setTimeout(timeoutMs);
    if (!http.begin(url))
        return String();
    int code = http.GET();
    String payload;
    if (code == 200)
        payload = http.getString();
    http.end();
    return payload;
}

// Parse Binance JSON – very small, avoid ArduinoJson for flash size
static bool parsePrice(const String &body, float &out)
{
    int idx = body.indexOf("\"price\":\"");
    if (idx < 0)
        return false;
    idx += 9; // skip to first digit
    int end = body.indexOf('"', idx);
    if (end < 0)
        return false;
    out = body.substring(idx, end).toFloat();
    return true;
}

// Fetch the symbols' current prices
static void fetchPrice()
{
    float focusPrice = prices;
    bool focusUpdate = false;

    float fetched = prices;
    bool ok = false;

    String url = String(BINANCE_API) + symbol;
    Serial.printf("[API] GET %s\n", url.c_str());

    String body = httpGET(url.c_str(), 3000);
    if (!body.isEmpty() && parsePrice(body, fetched))
    {
        Serial.printf("[API] OK  -> %s %.2f\n", symbol, fetched);
        if (openPrices == 0)
            openPrices = fetched; // capture session open once
        lastApiMs = millis();
        ok = true;
    }
    else
    {
        Serial.printf("[API] ERR -> %s\n", symbol);
    }
    if (ok){
      prices = fetched;      
    }

}

void displayIntro() {
 
  WiFi.begin(ssid, password);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.setCursor(4, 15);
  u8g2.print("Connecting...");
  u8g2.sendBuffer();
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
  u8g2.clearBuffer();
  u8g2.setCursor(4, 15);
  u8g2.print("Connected!!!");
  u8g2.setCursor(4, 22);
  u8g2.print(WiFi.localIP());
  u8g2.sendBuffer();

}
