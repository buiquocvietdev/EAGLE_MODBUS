// Automatic door
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <ModbusIP_ESP8266.h>
// Address of Registers
#define adrOpen 1
#define adrClose 2
#define adrOpenState 3
#define adrCloseState 4
#define TRUE 1
#define FALSE 0

#define LEDSTATUS 12
#define DOOR 14 

#define ON LOW
#define OFF HIGH

IPAddress remote(192, 168, 1, 7);  // Address of Modbus Slave device
IPAddress local_IP(192, 168, 1, 88); // IP DOOR
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

ModbusIP mb;

uint16_t bitOpen, bitClose, bitStatusOpen, bitStatusClose = 0;

void setup() {
  Serial.begin(115200);

  // Set static IP for ESP8266
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin("Robot", "1234567890@");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(">>");
  }

  Serial.println("");
  Serial.println("Connected IP address: ");
  Serial.println(WiFi.localIP());

  mb.server();          // Enable server mode for Modbus
  mb.addHreg(0, 0, 5); 
  Serial.println("Port 502 opened!");

  pinMode(LEDSTATUS, OUTPUT);
  pinMode(DOOR, OUTPUT);
  digitalWrite(LEDSTATUS, HIGH);
  digitalWrite(DOOR, LOW);
}

void loop() {
  mb.task();
  delay(300);

  bitOpen = mb.Hreg(adrOpen);
  bitClose = mb.Hreg(adrClose);

  //bitStatusOpen = mb.Hreg(adrOpenState);
  //bitStatusClose = mb.Hreg(adrCloseState);

  if (bitOpen == TRUE) { // read  bit open door 
    digitalWrite(DOOR, HIGH);
    mb.Hreg(adrOpenState, 1); // return the opened status bit
    delay(10);
  } else if (bitClose == TRUE) { // read bit close door
    digitalWrite(DOOR, LOW);
    mb.Hreg(adrCloseState, 1); //return the closed status bit
    delay(10);
  }
  // Device Event will reset all bit state to 0.
}
