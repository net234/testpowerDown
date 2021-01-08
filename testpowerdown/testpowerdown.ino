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

////ENABLE_BP0TORESET (TXD) GPIO2 D4 is used as ENABLE BP0 to reset
////ENABLE_BP0TORESET (FLASH) GPIO0 D3 is used as ENABLE BP0 to reset
////ENABLE_BP0TORESET (MTD0) GPIO15 D8 is used as ENABLE BP0 to reset
////ENABLE_BP0TORESET (WAKE) GPIO16 D0 is used as ENABLE BP0 to reset
//#define LOCK  LOW
//#define ENABLE_BP0TORESET 15

//BP_0 (MOSI)   D7   GPIO13 is Used as BP0 status (pullup)
#define BP_0 13




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


rst_info* resetInfoPtr;

//struct savedMemory_t {
//  char topTxt[16];
//  uint16_t compteur;
//  char bottomTxt[16];
//};
//savedMemory_t savedMemory __attribute__ ((section (".noinit")));

struct savedRTCmemory_t {
  float     checkPI;   // initialised to PI value to check POWER_ON Boot
  uint32_t  bootCounter;
  bool      RepeatPowerDown;
} savedRTCmemory;



bool bp0Status;
void setup() {
  // get BP_0 to know if it is a SLEEP_AWAKE or SLEEP_AWAKE_ABORTED
  pinMode(BP_0, INPUT);
  bp0Status = digitalRead(BP_0);
  // get reset reason
  resetInfoPtr = ESP.getResetInfoPtr();
  if (bp0Status == LOW && resetInfoPtr->reason == REASON_DEEP_SLEEP_AWAKE) resetInfoPtr->reason = REASON_DEEP_SLEEP_AWAKE_ABORTED;
  //Arduino/tools/sdk/include/user_interface.h 
  //     system_rtc_mem_write(10, &compteur, 2); //offset is 65
  Serial.println(F( "\r\n" APP_VERSION ));
//  if (resetInfoPtr->reason == REASON_DEEP_SLEEP_AWAKE ) {
////    Serial.println(F("Retourne PowerDown 5 sec"));
//    ESP.deepSleep(5 * 1E6, RF_DISABLED);
//  }

  

// init Serial
  Serial.begin(115200);
  Serial.println();
  if (resetInfoPtr->reason == REASON_DEEP_SLEEP_AWAKE ) {
    Serial.println(F("-> PowerDown 60 min"));
    ESP.deepSleep(60 * 60 * 1E6 - 187000, RF_DISABLED);  //- 187000 for 10  sec   -06m02s  pour 1H
    
    while (true) delay(1);
  }

  //system_rtc_mem_read(10, &savedRTCmemory, sizeof(savedRTCmemory));
  ESP.rtcUserMemoryRead(0,(uint32_t*)&savedRTCmemory, sizeof(savedRTCmemory));
  if (savedRTCmemory.checkPI != float(PI)) {
    Serial.println("Power on boot");
    savedRTCmemory.checkPI = PI;
    savedRTCmemory.bootCounter = 0;
    savedRTCmemory.RepeatPowerDown = false;
  } else {
    savedRTCmemory.bootCounter++;
    if (bp0Status == LOW ) savedRTCmemory.RepeatPowerDown = false;
  }
  ESP.rtcUserMemoryWrite(0,(uint32_t*)&savedRTCmemory, sizeof(savedRTCmemory));
  //system_rtc_mem_write(10, &savedRTCmemory, sizeof(savedRTCmemory));
  Serial.print("savedRTCmmemory.bootCounter = ");
  Serial.println(savedRTCmemory.bootCounter);


  
  switch (resetInfoPtr->reason) {
    case REASON_DEFAULT_RST:  Serial.println(F("->Cold boot")); break;
    case REASON_EXT_SYS_RST:  Serial.println(F("->boot with BP Reset")); break;
    case REASON_DEEP_SLEEP_AWAKE:  Serial.println(F("->boot from a deep sleep")); break;
    case REASON_DEEP_SLEEP_AWAKE_ABORTED: Serial.println(F("->boot from a deep sleep aborted with BP User")); break;
    default:
      Serial.print(F("->boot reason = "));
      Serial.println(resetInfoPtr->reason);
  }

  Serial.println(F( "\r\n" APP_VERSION ));
  if (resetInfoPtr->reason == REASON_DEEP_SLEEP_AWAKE ) {
    Serial.println(F("Retourne PowerDown 5 sec"));
    ESP.deepSleep(5 * 1E6, RF_DISABLED);
  }
  //  // check saved memory
  //  String aString = APP_VERSION;
  //  if ( aString.startsWith(savedMemory.topTxt) && aString.startsWith(savedMemory.bottomTxt) ) {
  //    Serial.println(F( "saved memory intact" ));
  //  } else {
  //    // init saved memory
  //    Serial.println(F( "Init saved memory" ));
  //    strncpy(savedMemory.topTxt, aString.c_str(), 15);
  //    savedMemory.topTxt[15] = 0;
  //    strncpy(savedMemory.bottomTxt, aString.c_str(), 15);
  //    savedMemory.bottomTxt[15] = 0;
  //
  //    savedMemory.compteur = 0;
  //  }
  //  if ( !(aString.startsWith(savedMemory.topTxt) && aString.startsWith(savedMemory.bottomTxt)) ) {
  //    // init saved memory
  //    Serial.println(F( "Bad Init saved memory" ));
  //  }

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
  if(resetInfoPtr->reason == REASON_DEEP_SLEEP_AWAKE) {
    
  }


  bp0Status = digitalRead(BP_0);
  Serial.print(("BP_0 = "));
  Serial.println(bp0Status);

  Serial.println(F("Bonjour ..."));
  //  Serial.println(resetInfoPtr->reason);
  //  if (resetInfoPtr->reason != REASON_DEEP_SLEEP_AWAKE) {
  //    savedMemory.compteur = 0;
  //  } else {
  //    savedMemory.compteur++;
  //  }
  Serial.print(F("compteur = "));
  Serial.println(savedRTCmemory.bootCounter);
  Serial.println(F("Type S for DeepSleep"));
  Serial.print(F("RTC Time="));
  Serial.println(system_get_rtc_time());
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
  }
  if ( bp0Status != digitalRead(BP_0) ) {
    bp0Status = !bp0Status;
    Serial.print(F("BP0 = "));
    Serial.println(bp0Status);
  }

  delay(10);
}

//uint32_t calculateCRC32(const uint8_t *data, size_t length) {
//  uint32_t crc = 0xffffffff;
//  while (length--) {
//    uint8_t c = *data++;
//    for (uint32_t i = 0x80; i > 0; i >>= 1) {
//      bool bit = crc & 0x80000000;
//      if (c & i) {
//        bit = !bit;
//      }
//      crc <<= 1;
//      if (bit) {
//        crc ^= 0x04c11db7;
//      }
//    }
//  }
//  return crc;
//}
////https://github.com/carCV/CalculateCRC/blob/master/crc32.h
//#define CRCPOLY_LE 0xedb88320
///// zlib's CRC32 polynomial
//  const uint32_t Polynomial = 0xEDB88320;
//u32  crc32_le(u32 crc, unsigned char const *p, unsigned int len)
//{
//    int i;
//    while (len--) {
//        crc ^= *p++;
//        for (i = 0; i < 8; i++)
//            crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
//    }
//    return crc;
//}
///// compute CRC32 (standard algorithm)
////https://github.com/stbrumme/crc32/blob/master/Crc32.cpp
//uint32_t crc32_1byte(const void* data, size_t length, uint32_t previousCrc32)
//{
//  uint32_t crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
//  const uint8_t* current = (const uint8_t*) data;
//
//  while (length-- != 0)
//    crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *current++];
//
//  return ~crc; // same as crc ^ 0xFFFFFFFF
//}
//#endif
