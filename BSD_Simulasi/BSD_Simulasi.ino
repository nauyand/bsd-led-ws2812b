#include <Adafruit_NeoPixel.h>
#include <TimerOne.h>

#define LED_PIN        2
#define NUM_LEDS       16
#define LEDS_PER_SIDE  8      // jumlah LED per sisi (kiri/kanan)
#define BUZZ_KIRI      9
#define BUZZ_KANAN     10
#define BRIGHTNESS     80     // kecerahan tetap (0-255)
#define MAX_DIST       24.0   // jarak terjauh yang masih menyalakan LED
#define AUTO_DEMO      true   // true = simulasi langsung jalan saat Arduino nyala

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

/* --- VARIABLE SISTEM & ANIMASI --- */
int currentLedsKiri = 0, currentLedsKanan = 0;
int targetKiri = 0, targetKanan = 0;
unsigned long lastUpdate = 0;
const int runningSpeed = 25;          // ms per langkah animasi LED
String inputBuffer = "";

/* --- VARIABLE BUZZER (TIMER1) --- */
volatile float distKiri = 0, distKanan = 0;
int counterKiri = 0, counterKanan = 0;

/* --- VARIABLE SIMULASI --- */
enum SimPhase { MENDEKAT, BERTAHAN, MENJAUH, JEDA };
bool demoMode = AUTO_DEMO;
int  demoSetting = 0;                 // 0 = acak, 1 = kiri, 2 = kanan, 3 = keduanya
int  simSide = 2;                     // sisi ronde ini: 1 = kiri, 2 = kanan, 3 = keduanya
SimPhase simPhase = JEDA;
float simDist = 30;                   // jarak objek saat ini
float simSpeed = 0.5;                 // pengurangan jarak tiap 100 ms
float simMinDist = 2;                 // jarak terdekat objek di ronde ini
unsigned long simPhaseStart = 0, simPhaseTime = 0, lastSimTick = 0;
const int SIM_TICK = 100;             // simulasi di-update tiap 100 ms

void setup() {
  Serial.begin(115200);
  pinMode(BUZZ_KIRI, OUTPUT);
  pinMode(BUZZ_KANAN, OUTPUT);
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();

  Timer1.initialize(100000); // 0.1 detik
  Timer1.attachInterrupt(buzzerHandler);

  randomSeed(analogRead(A0));  // A0 kosong -> nilai acak

  Serial.println("Sistem BSD: 16 LED (8 kiri / 8 kanan) pin 2 - Mega");
  Serial.println("Perintah: demo | kiri | kanan | dua | stop | atau data 'kiri,0,kanan,0'");
  if (demoMode) startNewRound();
}

/* ================= BUZZER ================= */
void buzzerHandler() {
  processBuzzer(distKiri, BUZZ_KIRI, counterKiri);
  processBuzzer(distKanan, BUZZ_KANAN, counterKanan);
}

void processBuzzer(float dist, int pin, int &counter) {
  if (dist > 0.01 && dist < 5.0) {
    digitalWrite(pin, HIGH);
  }
  else if (dist >= 5.0 && dist < 10.0) {
    counter++;
    digitalWrite(pin, counter <= 2 ? HIGH : LOW);
    if (counter >= 5) counter = 0;
  }
  else if (dist >= 10.0 && dist <= 15.0) {
    counter++;
    digitalWrite(pin, counter <= 2 ? HIGH : LOW);
    if (counter >= 10) counter = 0;
  }
  else {
    digitalWrite(pin, LOW);
    counter = 0;
  }
}

/* ================= LED ================= */
// Gradasi hijau -> kuning -> merah, otomatis menyesuaikan LEDS_PER_SIDE
uint32_t getFullGradient(int i) {
  float t = (float)i / (LEDS_PER_SIDE - 1);   // 0.0 .. 1.0
  int r, g;
  if (t < 0.3)      { r = 255 * t / 0.3;  g = 255; }          // hijau -> kuning
  else if (t < 0.9) { r = 255; g = 255 * (0.9 - t) / 0.6; }   // kuning -> merah
  else              { r = 255; g = 0; }                       // ujung merah murni
  return strip.Color(r, g, 0);
}

void updateVisualBSD() {
  if (currentLedsKiri < targetKiri) currentLedsKiri++;
  else if (currentLedsKiri > targetKiri) currentLedsKiri--;

  if (currentLedsKanan < targetKanan) currentLedsKanan++;
  else if (currentLedsKanan > targetKanan) currentLedsKanan--;

  strip.clear();
  for (int i = 0; i < currentLedsKiri; i++)  strip.setPixelColor(i, getFullGradient(i));
  for (int i = 0; i < currentLedsKanan; i++) strip.setPixelColor(NUM_LEDS - 1 - i, getFullGradient(i));
  strip.show();
}

// Animasi LED berjalan tanpa menghentikan program (non-blocking)
void animateLeds() {
  if ((currentLedsKiri != targetKiri || currentLedsKanan != targetKanan) &&
      millis() - lastUpdate > runningSpeed) {
    lastUpdate = millis();
    updateVisualBSD();
  }
}

// Jarak 0.3..24 -> 1..LEDS_PER_SIDE LED; <=0.3 -> penuh; 0 / >24 -> mati
int distToLeds(float d) {
  if (d > 0 && d <= 0.3) return LEDS_PER_SIDE;
  if (d > 0.3 && d <= MAX_DIST)
    return constrain((int)ceil((MAX_DIST - d) * LEDS_PER_SIDE / MAX_DIST), 1, LEDS_PER_SIDE);
  return 0;
}

// Pusat semua input jarak (dari Serial maupun simulasi)
void setDistances(float kiri, float kanan, bool alwaysPrint) {
  distKiri = kiri;
  distKanan = kanan;
  int tk = distToLeds(kiri);
  int tn = distToLeds(kanan);

  if (alwaysPrint || tk != targetKiri || tn != targetKanan) {
    Serial.print("Kiri: ");   Serial.print(kiri);  Serial.print(" -> "); Serial.print(tk);
    Serial.print(" LED | Kanan: "); Serial.print(kanan); Serial.print(" -> "); Serial.print(tn);
    Serial.println(" LED");
  }
  targetKiri = tk;
  targetKanan = tn;
}

/* ================= SIMULASI ================= */
const char* sideName(int s) {
  if (s == 1) return "KIRI";
  if (s == 2) return "KANAN";
  return "KIRI & KANAN";
}

void startNewRound() {
  simSide    = (demoSetting == 0) ? random(1, 4) : demoSetting;  // acak 1..3
  simDist    = MAX_DIST + 4;                    // mulai dari luar jangkauan
  simSpeed   = random(2, 9) / 10.0;             // 0.2..0.8 per 100 ms (2..8 per detik)
  simMinDist = random(5, 60) / 10.0;            // jarak terdekat 0.5..5.9
  simPhase   = MENDEKAT;

  Serial.print("--- Simulasi: objek mendekat dari "); Serial.print(sideName(simSide));
  Serial.print(" | kecepatan "); Serial.print(simSpeed * 10); Serial.print("/detik");
  Serial.print(" | jarak terdekat "); Serial.println(simMinDist);
}

void runSimulation() {
  if (millis() - lastSimTick < SIM_TICK) return;
  lastSimTick = millis();

  switch (simPhase) {
    case MENDEKAT:
      simDist -= simSpeed;
      if (simDist <= simMinDist) {
        simDist = simMinDist;
        simPhase = BERTAHAN;
        simPhaseStart = millis();
        simPhaseTime = random(1000, 3000);      // diam di dekat 1..3 detik
      }
      break;

    case BERTAHAN:
      if (millis() - simPhaseStart > simPhaseTime) simPhase = MENJAUH;
      break;

    case MENJAUH:
      simDist += simSpeed;
      if (simDist > MAX_DIST + 2) {
        setDistances(0, 0, false);
        simPhase = JEDA;
        simPhaseStart = millis();
        simPhaseTime = random(1000, 2500);      // jeda 1..2.5 detik sebelum objek berikutnya
        return;
      }
      break;

    case JEDA:
      if (millis() - simPhaseStart > simPhaseTime) startNewRound();
      return;
  }

  float kiri  = (simSide == 1 || simSide == 3) ? simDist : 0;
  float kanan = (simSide == 2 || simSide == 3) ? simDist : 0;
  setDistances(kiri, kanan, false);
}

void startDemo(int setting) {
  demoMode = true;
  demoSetting = setting;
  startNewRound();
}

/* ================= SERIAL ================= */
void processData(String data) {
  int comma1 = data.indexOf(',');
  int comma2 = data.indexOf(',', comma1 + 1);
  int comma3 = data.indexOf(',', comma2 + 1);

  if (comma1 > 0 && comma3 > 0) {
    demoMode = false;   // data manual masuk -> simulasi berhenti
    float kiri  = data.substring(0, comma1).toFloat();
    float kanan = data.substring(comma2 + 1, comma3).toFloat();
    setDistances(kiri, kanan, true);
  } else {
    Serial.println("Format salah. Contoh: 12,0,0,0  atau ketik: demo / kiri / kanan / dua / stop");
  }
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.length() == 0) return;

  if      (cmd == "demo")  startDemo(0);
  else if (cmd == "kiri")  startDemo(1);
  else if (cmd == "kanan") startDemo(2);
  else if (cmd == "dua")   startDemo(3);
  else if (cmd == "stop") {
    demoMode = false;
    setDistances(0, 0, true);
    Serial.println("Simulasi berhenti.");
  }
  else processData(cmd);
}

void handleSerial() {
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      handleCommand(inputBuffer);
      inputBuffer = "";
    } else {
      inputBuffer += inChar;
    }
  }
}

void loop() {
  handleSerial();
  if (demoMode) runSimulation();
  animateLeds();
}
