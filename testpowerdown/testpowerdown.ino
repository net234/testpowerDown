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
//ENABLE_BP0TORESET (TXD) GPIO2 D4 is used as ENABLE BP0 to reset
//ENABLE_BP0TORESET (FLASH) GPIO0 D3 is used as ENABLE BP0 to reset
//ENABLE_BP0TORESET (MTD0) GPIO15 D8 is used as ENABLE BP0 to reset
//ENABLE_BP0TORESET (WAKE) GPIO16 D0 is used as ENABLE BP0 to reset
#define LOCK  LOW
#define ENABLE_BP0TORESET 15
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

rst_info* resetInfoPtr;

struct savedMemory_t {
  char topTxt[16];
  uint16_t compteur;
  char bottomTxt[16];
};
savedMemory_t savedMemory __attribute__ ((section (".noinit")));
bool bp0Status;
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
  pinMode(BP_0, INPUT);
  bp0Status = digitalRead(BP_0);
  Serial.print(("BP_0="));
  Serial.println(bp0Status);
  delay(10);
  bp0Status = digitalRead(BP_0);
  Serial.print(("BP_0="));
  Serial.println(bp0Status);


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
  digitalWrite(ENABLE_BP0TORESET, LOCK );
  pinMode(ENABLE_BP0TORESET, OUTPUT);

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

  bp0Status = digitalRead(BP_0);
  Serial.print(("BP_0="));
  Serial.println(bp0Status);



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
  bp0Status = !digitalRead(BP_0);
}

void loop() {
  if (Serial.available()) {
    char aChar = (char)Serial.read();
    if (aChar == 'S') {

      //  if (resetInfoPtr->reason == REASON_EXT_SYS_RST || resetInfoPtr->reason == REASON_DEEP_SLEEP_AWAKE) {
//      Serial.println(F("---->DeepSleep"));
//      pinMode(ENABLE_BP0TORESET, OUTPUT);
//      //pinMode(ENABLE_BP0TORESET, INPUT);
//
//      digitalWrite(ENABLE_BP0TORESET, !LOCK );
      Serial.println(F("---->Enable BP0 to reset"));
      ESP.deepSleep(5E6, RF_DISABLED);
      //      WiFi.suspend();
      //      WiFi.resume();
      int compteur;
      system_rtc_mem_write(65, &compteur, 2); //offset is 65
      system_rtc_mem_read(65, &compteur, 2); //offset is 65
    }
    if (aChar == 'U') {

      Serial.println(F("---->Unlock"));
      pinMode(ENABLE_BP0TORESET, OUTPUT);
      //pinMode(ENABLE_BP0TORESET, INPUT);

      digitalWrite(ENABLE_BP0TORESET, !LOCK );
    }
    if (aChar == 'L') {
      Serial.println(F("---->Lock"));
      pinMode(ENABLE_BP0TORESET, OUTPUT);
      //pinMode(ENABLE_BP0TORESET, INPUT);

      digitalWrite(ENABLE_BP0TORESET, LOCK );
    }
  }
  if ( bp0Status != digitalRead(BP_0) ) {
    bp0Status = !bp0Status;
    Serial.print(F("BP0="));
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
