#include <WiFi.h>
#include <HTTPClient.h>
#define LM35_PIN   34
#define IR_PIN     27
#define VIB_PIN    26
#define BUZZER_PIN 25
const char* SSID     = "Darshan";
const char* PASSWORD = "0987654321";
const char* SERVER   = "http://10.155.111.160:8080/data";
volatile uint32_t pulseCount = 0;
unsigned long lastRPMTime    = 0;
uint32_t currentRPM          = 0;
void IRAM_ATTR rpmISR() 
{
  pulseCount++;
}
float getTemp() {
  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(LM35_PIN);
    delay(1);
  }
  float avg     = sum / 10.0;
  float voltage = avg * (3.3 / 4095.0);
  float temp    = voltage * 100.0;
  return temp;
}
uint32_t getRPM()
{
  unsigned long now     = millis();
  unsigned long elapsed = now - lastRPMTime;

  if (elapsed >= 1000) {
    // pulses in last 1 second → RPM
    currentRPM  = (pulseCount * 60000UL) / elapsed;
    pulseCount  = 0;
    lastRPMTime = now;
  }
  return currentRPM;
}
String getVibration() {
  return digitalRead(VIB_PIN) == HIGH ? "YES" : "NO";
}
String getRisk(float temp, uint32_t rpm, String vib) {
  bool t = temp > 60;
  bool r = rpm  > 3600;
  bool v = vib  == "YES";

  if (t && r && v) return "HIGH ALERT";
  if (t && r)      return "ALERT";
  if (t || r || v) return "WARNING";
  return "NORMAL";
}
void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(IR_PIN,     INPUT_PULLUP);
  pinMode(VIB_PIN,    INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(IR_PIN), rpmISR, RISING);

  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected: " + WiFi.localIP().toString());
  Serial.println("─────────────────────────────────────");

  lastRPMTime = millis();
}
void loop() {
  float    temp = getTemp();
  uint32_t rpm  = getRPM();
  String   vib  = getVibration();
  String   risk = getRisk(temp, rpm, vib);
  Serial.println("─────────────────────────────────────");
  Serial.print  ("  Temperature : "); Serial.print(temp, 1); Serial.println(" °C");
  Serial.print  ("  RPM         : "); Serial.println(rpm);
  Serial.print  ("  Vibration   : "); Serial.println(vib);
  Serial.print  ("  Risk        : "); Serial.println(risk);
  if      (risk == "HIGH ALERT") { tone(BUZZER_PIN, 1000); }
  else if (risk == "ALERT")      { tone(BUZZER_PIN, 500);  }
  else                           { noTone(BUZZER_PIN);     }
  if (WiFi.status() == WL_CONNECTED) 
  {
    HTTPClient http;
    http.begin(SERVER);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(1500);
    String body = "{";
    body += "\"temp\":"       + String(temp, 1) + ",";
    body += "\"rpm\":"        + String(rpm)     + ",";
    body += "\"vibration\":\"" + vib            + "\",";
    body += "\"risk\":\""     + risk            + "\"";
    body += "}";
    http.POST(body);
    http.end();
  }
  delay(500);
}