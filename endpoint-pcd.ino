#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
//#include <SoftwareSerial.h>

String datalogger = "Iniciando";

static const PROGMEM u1_t NWKSKEY[16] = {0xf1,0xdb,0xe6,0x27,0x22,0x33,0x52,0x1a,0x90,0x69,0x9a,0x98,0x42,0xbb,0xf9,0xbb};
static const u1_t PROGMEM APPSKEY[16] = {0xf1,0xdb,0xe6,0x27,0x22,0x33,0x52,0x1a,0x90,0x69,0x9a,0x98,0x42,0xbb,0xf9,0xbb};
static const u4_t DEVADDR = 0x065a35e1; // <-- Change this address for every node!

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

const unsigned TX_INTERVAL = 300;
int ttl = 0;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 3, 4},
};

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      break;
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK) {
        Serial.println(F("Received ack"));
        ttl = 0;
      } else {
        if (ttl < 5) {
          ttl++;
          Serial.print("TTL: ");
          Serial.println(ttl);
          os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(1), do_send);
        }
      }
      if (LMIC.dataLen) {
        Serial.println(F("Received "));
        Serial.print(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
      }
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    if(datalogger != "Iniciando") {
      LMIC_setTxData2(1,(unsigned char *) (datalogger.c_str()) , datalogger.length(), 1);
      Serial.println(F("Packet queued"));  
    }
  }
}


void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  Serial.println(F("Starting"));

  #ifdef VCC_ENABLE

    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);

  #endif

    os_init();
    LMIC_reset();

  #ifdef PROGMEM

    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);

  #else

    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);

  #endif

    LMIC_selectSubBand(1);
    LMIC_setLinkCheckMode(0);
    LMIC.dn2Dr = DR_SF9;
    LMIC_setDrTxpow(DR_SF10, 14);
    LMIC.errcr = CR_4_8;
    do_send(&sendjob);
}


void loop() {
  int i=0;
  String c;
  
  if(Serial1.available()){
    c = Serial1.readString();
    i = c.indexOf("9971");
    if(i!=-1){
      datalogger = c.substring(i);
      datalogger.replace("\n"," ");
      Serial.println("Serial is avaiable...");
      Serial.println(datalogger);
    }
  }
  os_runloop_once();
}
