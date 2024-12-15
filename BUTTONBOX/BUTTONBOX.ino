#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <ModbusIP_ESP8266.h>

// Heap threshold
#define HEAP_THRESHOLD 5000
// Address of registers
#define adrButton 1
#define adrLight 2
// I/O pin
#define buttonPin 2
#define ledPin 12
#define buzzPin 14
#define lockButton 5000  // 5 seconds cooldown
#define debounceDelay 50
// Configure IP
IPAddress remote(192, 168, 1, 7);     // IP Server
IPAddress local_IP(192, 168, 1, 84);  // Local IP of device // change it: 81 --> 87
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

ModbusIP mb;

bool buttonState = HIGH;
bool lastButtonState = LOW;
uint32_t lastDebounceTime = 0;
uint32_t pressStartTime = 0;
uint32_t lastButtonPressTime = 0;

bool isClick = false;
bool isPress = false;
bool isReset = false;
bool holdHandled = false;

uint16_t hregButton, hregLight = 0;  // variable temp of registers

enum alarmMode {
  MODE_SINGLE,
  MODE_DOUBLE,
  MODE_FAST
};
  
void setup() {
  Serial.begin(115200);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Failed to configure");
  }
  WiFi.begin("Robot", "1234567890@");  // SSID and password //change it

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(">>");
  }
  Serial.println("");
  Serial.println("Connected with IP address: ");
  Serial.println(WiFi.localIP());

  mb.server();          // Enable server mode for Modbus with port 502 (default in lib)
  mb.addHreg(0, 0, 3);  // (start hreg, value,  num of reg)

  Serial.println("Port 502 opened!");

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  digitalWrite(buzzPin, HIGH);
  delay(100);
  digitalWrite(buzzPin, LOW);
}

void loop() {
  checkheap();

  bool reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW && (millis() - lastButtonPressTime >= lockButton)) {
        pressStartTime = millis();
        holdHandled = false;
        isClick = false;
        isPress = false;
        isReset = false;
      } else {
        uint32_t pressDuration = millis() - pressStartTime;
        if (!holdHandled) {
          if (pressDuration < 500) { // single click
            isClick = true;
            holdHandled = true;
          } else if (pressDuration >= 500 && pressDuration < 4000) { // long press 
            isPress = true;
            holdHandled = true;
          }
        }
      }
    }
  }

  if (buttonState == LOW && !holdHandled) {
    uint32_t pressDuration = millis() - pressStartTime;
    if (pressDuration >= 10000 && digitalRead(buttonPin) == LOW) {
      isReset = true;
      holdHandled = true;
    }
  }

  mb.task();
  // Register after event completion
  hregLight = mb.Hreg(adrLight);
  if (hregLight == 1) {
    mb.Hreg(adrLight, 0);
    Serial.print("Event completed, done reset bit request: ");
    Serial.println(mb.Hreg(adrLight));

    alarm(MODE_SINGLE, 200);
  }
  // Active single click command
  if (isClick) {
    lastButtonPressTime = millis();
    mb.Hreg(adrButton, 1);
    hregButton = mb.Hreg(adrButton);
    Serial.print("Activated single click event: ");
    Serial.println(hregButton);

    alarm(MODE_SINGLE, 200);

    isClick = false;
  }
  // Active long press command
  if (isPress) {
    lastButtonPressTime = millis();
    mb.Hreg(adrButton, 2);
    hregButton = mb.Hreg(adrButton);
    Serial.print("Activated long press event: ");
    Serial.println(hregButton);

    alarm(MODE_DOUBLE, 200);

    isPress = false;
  }
  // Active reset command
  if (isReset) {
    isReset = false;
    ESP.restart();
  }

  lastButtonState = reading;
}
// Check heap and restart esp when full memory
void checkheap() {
  uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < HEAP_THRESHOLD) {
    Serial.println("Restarting...");
    ESP.restart();
  }
}
// Alram LED and Buzzer
void alarm(alarmMode mode, uint16_t timeDelay) {
  switch (mode) {
    case MODE_SINGLE:
      digitalWrite(ledPin, LOW);
      digitalWrite(buzzPin, HIGH);
      delay(timeDelay);
      digitalWrite(ledPin, HIGH);
      digitalWrite(buzzPin, LOW);
      break;

    case MODE_DOUBLE:
      for (int i = 0; i < 2; i++) {
        digitalWrite(ledPin, LOW);
        digitalWrite(buzzPin, HIGH);
        delay(timeDelay);
        digitalWrite(ledPin, HIGH);
        digitalWrite(buzzPin, LOW);
        delay(timeDelay);
      }
      break;

    case MODE_FAST:  //no need timeDelay
      for (int i = 0; i < 5; i++) {
        digitalWrite(ledPin, LOW);
        digitalWrite(buzzPin, HIGH);
        delay(100);
        digitalWrite(ledPin, HIGH);
        digitalWrite(buzzPin, LOW);
        delay(100);
      }
      break;
  }
}
