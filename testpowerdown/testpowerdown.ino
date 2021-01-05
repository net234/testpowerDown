/*************************
   Test power down
   net234 04/01/2021

   0001 simple test ESP.deepSleep(time_in_us)


*/

#include <Arduino.h>
#include <ESP8266WiFi.h>

#define APP_VERSION   "TestPowerDown 0001"

// GPIO2 on ESP32
#define LED_1  LED_BUILTIN
#define GPIO16  16
#define LED_ON LOW
#define LED_OFF (!LED_ON)


void setup() {
  // put your setup code here, to run once:
  // init Serial
  Serial.begin(115200);
  Serial.println(F("\r\n"APP_VERSION));

  pinMode( LED_1, OUTPUT );
  //pinMode( LED_2, OUTPUT );
  //digitalWrite( LED_1 , LED_ON );
  digitalWrite( LED_1 , LED_ON );


  // Wifi off
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();


  Serial.println(F("Bonjour ..."));
}

void loop() {
  Serial.println('A');
  digitalWrite( LED_1, LED_OFF );
  ESP.deepSleep(2000000,RF_DISABLED);
  digitalWrite( LED_1, LED_ON );
  Serial.println('B');
  delay(100);
}
