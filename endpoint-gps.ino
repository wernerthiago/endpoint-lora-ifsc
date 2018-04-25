#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
//GPS_Begin
#include <SoftwareSerial.h>
#include <TinyGPS.h>

TinyGPS gps;
SoftwareSerial ss(7, 6);

static void smartdelay(unsigned long ms);
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);
//GPS_End

static const PROGMEM u1_t NWKSKEY[16] = { 0xf1, 0xd5, 0x26, 0xca, 0xab, 0x26, 0x2b, 0x66, 0x19, 0x0a, 0x3f, 0xc4, 0xf9, 0x64, 0x85, 0xf6 };
static const u1_t PROGMEM APPSKEY[16] = { 0xf1, 0xd5, 0x26, 0xca, 0xab, 0x26, 0x2b, 0x66, 0x19, 0x0a, 0x3f, 0xc4, 0xf9, 0x64, 0x85, 0xf6 };
static const u4_t DEVADDR = 0x07aa69a1;

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

const unsigned TX_INTERVAL = 60;

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
    //GPS_Begin
    float flat, flon;
    unpair(flat, flon) = getVectorData();
    String latitude = String(flat, 5);
    latitude.remove(0,4);
    String longitude = String(flon, 5);
    longitude.remove(0,4);
    String aux = latitude + "," + longitude;
    //StringToBinary_Begin
    byte binary[aux.length()];
    for (int i = 0; i < aux.length(); i++)
    {
      binary[i] = byte(aux[i]);
    }
    //StringToBinary_End
    //GPS_End
    LMIC_setTxData2(1, binary, sizeof(binary) - 1, 1);
    Serial.println(F("Packet queued"));
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting"));
  //GPS_Begin
  ss.begin(9600);
  //GPS_End

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
  os_runloop_once();
}

static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void print_float(float val, float invalid, int len, int prec)
{
  if (val == invalid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i = flen; i < len; ++i)
      Serial.print(' ');
  }
  smartdelay(0);
}

static void print_int(unsigned long val, unsigned long invalid, int len)
{
  char sz[32];
  if (val == invalid)
    strcpy(sz, "*******");
  else
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i = strlen(sz); i < len; ++i)
    sz[i] = ' ';
  if (len > 0)
    sz[len - 1] = ' ';
  Serial.print(sz);
  smartdelay(0);
}

static void print_date(TinyGPS &gps)
{
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age == TinyGPS::GPS_INVALID_AGE)
    Serial.println("GPS_INVALID_AGE ");
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d ",
            month, day, year, hour, minute, second);
    Serial.print(sz);
  }
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  smartdelay(0);
}

static void print_str(const char *str, int len)
{
  int slen = strlen(str);
  for (int i = 0; i < len; ++i)
    Serial.print(i < slen ? str[i] : ' ');
  smartdelay(0);
}

template <typename T1, typename T2>
struct t_unpair
{
  T1& a1;
  T2& a2;
  explicit t_unpair( T1& a1, T2& a2 ): a1(a1), a2(a2) { }
  t_unpair<T1,T2>& operator = (const pair<T1,T2>& p)
    {
    a1 = p.first;
    a2 = p.second;
    return *this;
    }
};

// Our functor helper (creates it)
template <typename T1, typename T2>
t_unpair<T1,T2> unpair( T1& a1, T2& a2 )
{
  return t_unpair<T1,T2>( a1, a2 );
}

// Our function that returns a pair
pair<int,float> dosomething( char c )
{
  return make_pair<int,float>( c*10, c*2.9 );
}

pair<float,float> getVectorData()
{
  int aux = 0;
  float flat[10], flon[10];
  unsigned long age;
  static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;

  while(aux < 10)
  {
    print_int(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5);
    print_int(gps.hdop(), TinyGPS::GPS_INVALID_HDOP, 5);
    gps.f_get_position(&flat[aux], &flon[aux], &age);
    aux++;
    smartdelay(10000);
  }
  return make_pair<float,float>(median(flat),median(flon));
}

float median(float[] num)
{
  float median;
  for(int i = 0; i < num.length(); i++)
  {
    median = ++num[i];
  }
  return median/num.length();
}
