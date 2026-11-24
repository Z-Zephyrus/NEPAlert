#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* AT_USER = "sandbox";
const char* AT_KEY  = "PASTE_YOUR_SANDBOX_API_KEY";
const char* PHONE   = "+234XXXXXXXXXX";   // number registered in the AT simulator

const int MAINS_PIN = 14;   // LOW = mains present
const int LOAD_PIN  = 34;
const int LED_PIN   = 26;

const float VOLTAGE = 230.0;     // assumed mains voltage
const float MAX_AMPS = 20.0;     // pot full-scale

bool mainsOn = true;
float energyWh = 0;
unsigned long lastSample = 0;

String urlEncode(const String& s) {
  String out;
  const char* hex = "0123456789ABCDEF";
  for (char c : s) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += c;
    else { out += '%'; out += hex[(c >> 4) & 15]; out += hex[c & 15]; }
  }
  return out;
}

void sendSMS(const String& msg) {
  if (WiFi.status() != WL_CONNECTED) { Serial.println("No Wi-Fi"); return; }

  String body = "username=" + String(AT_USER) +
                "&to=" + urlEncode(PHONE) +
                "&message=" + urlEncode(msg);

  for (int attempt = 1; attempt <= 3; attempt++) {
    WiFiClientSecure client;
    client.setInsecure();                 // fine for simulation only
    client.setTimeout(15);
    HTTPClient http;
    http.setConnectTimeout(15000);
    http.setTimeout(15000);
    http.setReuse(false);
    http.begin(client, "https://api.sandbox.africastalking.com/version1/messaging");
    http.addHeader("apiKey", AT_KEY);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Accept", "application/json");

    int code = http.POST(body);
    Serial.printf("Attempt %d, status: %d (%s)\n", attempt, code,
                  code < 0 ? http.errorToString(code).c_str() : "ok");
    if (code > 0) {
      Serial.println(http.getString());
      http.end();
      return;
    }
    http.end();
    delay(2000);
  }
  Serial.println("SMS failed after 3 attempts");
}

float readAmps() {
  return analogRead(LOAD_PIN) / 4095.0 * MAX_AMPS;
}

void setup() {
  Serial.begin(115200);
  pinMode(MAINS_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) { delay(250); Serial.print("."); }
  Serial.println("\nWi-Fi connected");
  mainsOn = (digitalRead(MAINS_PIN) == LOW);
  digitalWrite(LED_PIN, mainsOn);
  Serial.printf("User: [%s]  Key length: %d\n", AT_USER, (int)strlen(AT_KEY));
}

void loop() {
  // --- mains state change detection with a 1 s debounce ---
  bool reading = (digitalRead(MAINS_PIN) == LOW);
  if (reading != mainsOn) {
    delay(1000);
    if ((digitalRead(MAINS_PIN) == LOW) == reading) {
      mainsOn = reading;
      digitalWrite(LED_PIN, mainsOn);
      String msg = mainsOn
        ? "Power restored. Energy used so far: " + String(energyWh, 1) + " Wh"
        : "Power outage detected. Switched to backup.";
      Serial.println(msg);
      sendSMS(msg);
    }
  }

  // --- power and energy once per second (only while mains is on) ---
  if (millis() - lastSample >= 1000) {
    lastSample = millis();
    if (mainsOn) {
      float amps = readAmps();
      float watts = VOLTAGE * amps;
      energyWh += watts / 3600.0;
      Serial.printf("I=%.2f A  P=%.0f W  E=%.2f Wh\n", amps, watts, energyWh);
    }
  }
}
