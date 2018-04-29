#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <SoftwareSerial.h>

#define NUM_COLETAS 4

String datalogger = "Iniciando";
SoftwareSerial mySerial(6,7);

float temperatura[NUM_COLETAS];
float radar[NUM_COLETAS];
float pluviometro[NUM_COLETAS];

static const PROGMEM u1_t NWKSKEY[16] = {0xf1,0xdb,0xe6,0x27,0x22,0x33,0x52,0x1a,0x90,0x69,0x9a,0x98,0x42,0xbb,0xf9,0xbb}
static const u1_t PROGMEM APPSKEY[16] = {0xf1,0xdb,0xe6,0x27,0x22,0x33,0x52,0x1a,0x90,0x69,0x9a,0x98,0x42,0xbb,0xf9,0xbb}
static const u4_t DEVADDR = 0x065a35e1;

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

const unsigned TX_INTERVAL = 300;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 8,
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
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
        os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      if (LMIC.dataLen) {
        Serial.println(F("Received "));
        Serial.println(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
      }
      // Schedule next transmission
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
    LMIC_setTxData2(1,(unsigned char *) (datalogger.c_str()) , datalogger.length(), 1);
    Serial.println(F("Packet queued"));
  }
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
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
  getData();
}

void getData() {
  int m = 0, k = 0, lastPosition = 0;
  float pluv = 0, temp = 0, rad = 0;
  String mensagem;
  String aux[7] = {};
  String c = "9971\n2018\n208\n1831\n12.70\n83\n2.00\n12.5";

  while (k < NUM_COLETAS) {
    if (mySerial.available()) {
      c = mySerial.readString();
      lastPosition = 0;
      m = 0;
      for (int j = 0; j < c.length() + 1; j++) {
        if (c.charAt(j) == '\n') {
          if (m == 0) {
            aux[m] = c.substring(lastPosition, j);
          } else {
            aux[m] = c.substring(lastPosition + 1, j);
          }
          lastPosition = j;
          m++;
        }
      }
      //Terminando de montar o array
      aux[m] = c.substring(lastPosition + 1, c.length());
      //Coletando as informacoes dos sensores
      temperatura[k] = aux[6].toFloat();
      radar[k] = aux[5].toFloat();
      pluviometro[k] = aux[4].toFloat();
      //Esperando para realizar a proxima coleta
      k++;
    } else {
      //Se nao estiver disponivel espera ate que esteja.
    }
    os_runloop_once();
  }
  //Fazendo as medias das coletas dos sensores
  aux[6] = median(temperatura);
  aux[5] = median(radar);
  aux[4] = maximum(pluviometro);

  //Montando a string que sera enviada atraves do LoRa
  for (int j = 0; j < 7; j++) {
    mensagem = mensagem + aux[j] + ",";
  }
  //Terminando de montar a string
  datalogger = mensagem + c.substring(lastPosition + 1, c.length());
}

String median(float num[]) {
  float aux = 0;
  for (int i = 0; i < sizeof(num); i++) {
    aux = aux + num[i];
  }
  return String(aux / sizeof(num));
}

String maximum(float array[]) {
  float mxm = array[0];
  for (int i = 0; i < sizeof(array); i++) {
    if (array[i] > mxm) {
      mxm = array[i];
    }
  }
  return String(mxm);
}
