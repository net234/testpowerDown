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
//LED_1 D4(GPIO2)   LED_BUILTIN HERE
//LED_2 D0(GPIO16)
#define LED_1  LED_BUILTIN
#define LED_ON LOW
#define LED_OFF (!LED_ON)

//BP_0 (MOSI)   D7   GPIO13 is Used as BP0 status (pullup)
#define BP_0 13
#define BP_0_DOWN LOW

#define POWER_ENABLE  16
#define PE_OFF LOW



//enum rst_reason {
// REASON_DEFAULT_RST = 0, /* normal startup by power on */
// REASON_WDT_RST = 1, /* hardware watch dog reset */
// REASON_EXCEPTION_RST = 2, /* exception reset, GPIO status won't change */
// REASON_SOFT_WDT_RST   = 3, /* software watch dog reset, GPIO status won't change */
// REASON_SOFT_RESTART = 4, /* software restart ,system_restart , GPIO status won't change */
// REASON_DEEP_SLEEP_AWAKE = 5, /* wake up from deep-sleep */
// REASON_EXT_SYS_RST      = 6 /* external system reset */
//};
#define REASON_DEEP_SLEEP_AWAKE_ABORTED  ( (rst_reason)10)



//struct savedMemory_t {
//  char topTxt[16];
//  uint16_t compteur;
//  char bottomTxt[16];
//};
//savedMemory_t savedMemory __attribute__ ((section (".noinit")));

struct savedRTCmemory_t {
  float     checkPI;       // initialised to PI value to check POWER_ON Boot
  uint32_t  bootCounter;   // Number of reboot since power on
  uint32_t  awakeCounter;  // Number of awake since last Deep Sleep
} savedRTCmemory;

uint8_t resetReason;  //resetInfoPtr->reason

bool bp0Status;
void setup() {
  // get BP_0 to know if it is a SLEEP_AWAKE or SLEEP_AWAKE_ABORTED
  //pinMode(BP_0, INPUT_PULLUP);
  bp0Status = digitalRead(BP_0);
  // get reset reason
  rst_info* resetInfoPtr = ESP.getResetInfoPtr();
  resetReason = resetInfoPtr->reason;
  if (bp0Status == LOW &&  resetReason == REASON_DEEP_SLEEP_AWAKE) resetReason = REASON_DEEP_SLEEP_AWAKE_ABORTED;




  // init Serial
  Serial.begin(115200);
  Serial.println(F( "\r\n" APP_VERSION ));
  //  Serial.print(F("RTC Time="));
  //  Serial.println(system_get_rtc_time());

  //system_rtc_mem_read(10, &savedRTCmemory, sizeof(savedRTCmemory));
  ESP.rtcUserMemoryRead(0, (uint32_t*)&savedRTCmemory, sizeof(savedRTCmemory));
  if (savedRTCmemory.checkPI != float(PI)) {
    Serial.println("Power on boot");
    savedRTCmemory.checkPI = PI;
    savedRTCmemory.bootCounter = 0;
    savedRTCmemory.awakeCounter = 0;
  } else {
    savedRTCmemory.bootCounter++;
    savedRTCmemory.awakeCounter++;
  }

  if (resetReason == REASON_DEEP_SLEEP_AWAKE ) {
    Serial.println(F("-> PowerDown 60 min"));
    ESP.rtcUserMemoryWrite(0, (uint32_t*)&savedRTCmemory, sizeof(savedRTCmemory));
    //system_rtc_mem_write(10, &savedRTCmemory, sizeof(savedRTCmemory));

    ESP.deepSleep(60 * 60 * 1E6 - 187000, RF_DISABLED);  //- 187000 for 10  sec   -06m02s  pour 1H

    while (true) delay(1);
  }
  // full awake so erase RTC awake Counter
  uint32_t awakeCounter = savedRTCmemory.awakeCounter;
  savedRTCmemory.awakeCounter = 0;
  ESP.rtcUserMemoryWrite(0, (uint32_t*)&savedRTCmemory, sizeof(savedRTCmemory));
  savedRTCmemory.awakeCounter = awakeCounter;

  Serial.print("savedRTCmmemory.bootCounter = ");
  Serial.println(savedRTCmemory.bootCounter);
  Serial.print("savedRTCmmemory.awakeCounter = ");
  Serial.println(savedRTCmemory.awakeCounter);


  switch (resetReason) {
    case REASON_DEFAULT_RST:  Serial.println(F("->Cold boot")); break;
    case REASON_EXT_SYS_RST:  Serial.println(F("->boot with BP Reset")); break;
    case REASON_DEEP_SLEEP_AWAKE:  Serial.println(F("->boot from a deep sleep")); break;
    case REASON_DEEP_SLEEP_AWAKE_ABORTED: Serial.println(F("->boot from a deep sleep aborted with BP User")); break;
    case REASON_SOFT_RESTART: Serial.println(F("->boot after a soft Reset")); break;
    default:
      Serial.print(F("->boot reason = "));
      Serial.println(resetReason);
  }

  Serial.println(F( APP_VERSION ));

  pinMode( LED_1, OUTPUT );
  //pinMode( LED_2, OUTPUT );
  //digitalWrite( LED_1 , LED_ON );
  digitalWrite( LED_1 , LED_ON );


  // Wifi off
  if ( WiFi.getMode() != WIFI_OFF)  {
    Serial.print(F("WiFiMode = "));
    Serial.println(WiFi.getMode());
  }
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  WiFi.forceSleepBegin();  // this do  a WiFiMode OFF  !!! 21ma
  // return to sleep


  bp0Status = digitalRead(BP_0);
  Serial.print(("BP_0 = "));
  Serial.println(bp0Status);

  Serial.println(F("Bonjour ..."));

  Serial.print(F("compteur = "));
  Serial.println(savedRTCmemory.bootCounter);
  Serial.println(F("Type S for DeepSleep"));
  //  Serial.print(F("RTC Time="));
  //  Serial.println(system_get_rtc_time());
  bp0Status = !digitalRead(BP_0);
}

void loop() {
  if (Serial.available()) {
    char aChar = (char)Serial.read();
    if (aChar == 'S') {
      Serial.print(F("PowerDown 5 sec"));
      ESP.deepSleep(5 * 1E6, RF_DISABLED);

    }
    if (aChar == 'L') {
      Serial.print(F("PowerDown 1 Hour"));
      ESP.deepSleep(60 * 60 * 1E6, RF_DISABLED);
    }

    if (aChar == 'H') {
      Serial.println(F("Hard reset"));
      delay(10);
      pinMode(POWER_ENABLE, OUTPUT);
      digitalWrite(POWER_ENABLE, PE_OFF);  //
      delay(1000);
      Serial.println(F("Soft Rest"));
      ESP.reset();
    }
    if (aChar == 'R') {
      Serial.print(F("Soft reset"));
      ESP.reset();
    }

  }
  static uint32_t lastDown = millis();
  if ( bp0Status != digitalRead(BP_0) ) {
    bp0Status = !bp0Status;
    Serial.print(F("BP0 = "));
    Serial.println(bp0Status);
    digitalWrite( LED_1 , LED_OFF );
    delay(100);
    digitalWrite( LED_1 , LED_ON );
    if (bp0Status == BP_0_DOWN) {
      lastDown = millis();
    } else {
      if ( millis() - lastDown  > 3000 ) {
        Serial.print(F("PowerDown 1 Hour"));
        ESP.deepSleep(60 * 60 * 1E6, RF_DISABLED);

      }
    }
  }

  delay(10);
}
