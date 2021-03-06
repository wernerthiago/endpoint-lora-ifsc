#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// LoRaWAN NwkSKey, network session key
// This is the default Semtech key, which is used by the early prototype TTN
// network.
static const PROGMEM u1_t NWKSKEY[16] = {0xa5, 0xd6, 0x6b, 0xd7, 0x67, 0x76, 0xb7, 0x4e, 0x61, 0xd9, 0x89, 0x6e, 0x61, 0x6b, 0x9a, 0x81};
static const u1_t PROGMEM APPSKEY[16] = {0xa5, 0xd6, 0x6b, 0xd7, 0x67, 0x76, 0xb7, 0x4e, 0x61, 0xd9, 0x89, 0x6e, 0x61, 0x6b, 0x9a, 0x81};
static const u4_t DEVADDR = 0x06b6e7ab; // <-- Change this address for every node!

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 300;
int ttl = 0;
String data = "Iniciando";


// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 8,
  .dio = {2, 3, 4}
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
        if (ttl < 10) {
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
    data = "Mensagem";
    LMIC_setTxData2(1, (unsigned char *) data.c_str(), data.length(), 1);
    Serial.println(F("Packet queued"));
  }
}


void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting"));
  ttl = 0;

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
  LMIC.dn2Dr = SF9;

#else
  // If not running an AVR with PROGMEM, just use the arrays directly
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
#endif

  LMIC_selectSubBand(1);

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7, 14);

  // Alterar taxa do código. Padrão 4/5.
  LMIC.errcr = CR_4_8;

  // Start job
  do_send(&sendjob);
}


void loop() {
  os_runloop_once();
}
