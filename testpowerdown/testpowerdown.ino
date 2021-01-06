/*************************
   Test power down
   net234 04/01/2021

   0001 simple test ESP.deepSleep(time_in_us)
    Memory dont survive a deep sleep (including section (".noinit") )
    after a deepsleep external system reset (REASON_EXT_SYS_RST) reset are seen as REASON_DEEP_SLEEP_AWAKE it logical but not obvious

*/

#include <Arduino.h>
#include <ESP8266WiFi.h>

#define APP_VERSION   "TestPowerDown 0001"

// GPIO2 on ESP32
#define LED_1  LED_BUILTIN
#define GPIO16  16
#define LED_ON LOW
#define LED_OFF (!LED_ON)

//enum rst_reason {
// REASON_DEFAULT_RST = 0, /* normal startup by power on */
// REASON_WDT_RST = 1, /* hardware watch dog reset */
// REASON_EXCEPTION_RST = 2, /* exception reset, GPIO status won't change */
// REASON_SOFT_WDT_RST   = 3, /* software watch dog reset, GPIO status won't change */
// REASON_SOFT_RESTART = 4, /* software restart ,system_restart , GPIO status won't change */
// REASON_DEEP_SLEEP_AWAKE = 5, /* wake up from deep-sleep */
// REASON_EXT_SYS_RST      = 6 /* external system reset */
//};

rst_info* resetInfoPtr;

struct savedMemory_t {
  char topTxt[16];
  uint16_t compteur;
  char bottomTxt[16];
};
savedMemory_t savedMemory __attribute__ ((section (".noinit")));

void setup() {
  resetInfoPtr = ESP.getResetInfoPtr();

  // init Serial
  Serial.begin(115200);
  Serial.println();
  switch (resetInfoPtr->reason) {
    case REASON_DEFAULT_RST:  Serial.println(F("->Cold boot")); break;
    case REASON_EXT_SYS_RST:  Serial.println(F("->boot with BP Reset")); break;
    case REASON_DEEP_SLEEP_AWAKE:  Serial.println(F("->boot from a deep sleep")); break;
    default:
      Serial.print(F("->boot reason = "));
      Serial.println(resetInfoPtr->reason);
  }


  
  Serial.println(F( "\r\n" APP_VERSION ));

  // check saved memory
  String aString = APP_VERSION;
  if ( aString.startsWith(savedMemory.topTxt) && aString.startsWith(savedMemory.bottomTxt) ) {
    Serial.println(F( "saved memory intact" ));
  } else {
    // init saved memory
    Serial.println(F( "Init saved memory" ));
    strncpy(savedMemory.topTxt, aString.c_str(), 15);
    savedMemory.topTxt[15] = 0;
    strncpy(savedMemory.bottomTxt, aString.c_str(), 15);
    savedMemory.bottomTxt[15] = 0;

    savedMemory.compteur = 0;
  }
  if ( !(aString.startsWith(savedMemory.topTxt) && aString.startsWith(savedMemory.bottomTxt)) ) {
    // init saved memory
    Serial.println(F( "Bad Init saved memory" ));
  }


  switch (resetInfoPtr->reason) {
    case REASON_DEFAULT_RST:  Serial.println(F("->Cold boot")); break;
    case REASON_EXT_SYS_RST:  Serial.println(F("->boot with BP Reset")); break;
    case REASON_DEEP_SLEEP_AWAKE:  Serial.println(F("->boot from a deep sleep")); break;
    default:
      Serial.print(F("->boot reason = "));
      Serial.println(resetInfoPtr->reason);
  }

  pinMode( LED_1, OUTPUT );
  //pinMode( LED_2, OUTPUT );
  //digitalWrite( LED_1 , LED_ON );
  digitalWrite( LED_1 , LED_ON );


  // Wifi off
  if ( WiFi.getMode() != WIFI_OFF)  {
    Serial.print(F("WiFiMode="));
    Serial.println(WiFi.getMode());
  }
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  WiFi.forceSleepBegin();  // this do  a WiFiMode OFF  !!! 21ma




  Serial.println(F("Bonjour ..."));
  Serial.println(resetInfoPtr->reason);
  if (resetInfoPtr->reason != REASON_DEEP_SLEEP_AWAKE) {
    savedMemory.compteur = 0;
  } else {
    savedMemory.compteur++;
  }
  Serial.print(F("compteur = "));
  Serial.println(savedMemory.compteur);
  Serial.println(F("Type S for DeepSleep"));
}

void loop() {
  if (Serial.available()) {
    char aChar = (char)Serial.read();
    if (aChar == 'S') {

      //  if (resetInfoPtr->reason == REASON_EXT_SYS_RST || resetInfoPtr->reason == REASON_DEEP_SLEEP_AWAKE) {
      Serial.println(F("---->DeepSleep"));
      digitalWrite( LED_1, LED_OFF );
      ESP.deepSleep(2000000, RF_DISABLED);
    }
    delay(100);
  }
}
