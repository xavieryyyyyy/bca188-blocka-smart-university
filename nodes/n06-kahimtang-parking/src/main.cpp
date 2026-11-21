// SENTRA - n06 KAHIMTANG parking - Serial dry run (diagnostic build)
// Reads two HC-SR04 sensors and prints the MQTT messages it would publish.
// No Wi-Fi or MQTT yet: the registry entries must be confirmed first.
// Every line ends with \r\n, which the Wokwi terminal needs.

#include <Arduino.h>

// ---- Pins (README pin map; each ECHO goes through its 2.2k / 3.3k divider) ----
const int TRIG_PIN[2] = {26, 32};
const int ECHO_PIN[2] = {27, 33};

// ---- Topics and metrics (DATA_REGISTRY.md) ----
const char* NODE_ID = "n06";
const char* METRIC[2] = {"bay-1-occupied", "bay-2-occupied"};
const char* TOPIC_BAY[2] = {
  "bca188/blocka/kahimtang/n06/bay-1-occupied",
  "bca188/blocka/kahimtang/n06/bay-2-occupied",
};
const char* TOPIC_STATUS = "bca188/blocka/kahimtang/n06/status";

// ---- Thresholds: PLACEHOLDERS until tested at the real mounting spots ----
const float OCCUPIED_BELOW_CM = 100.0;  // closer than this = occupied
const float FREE_ABOVE_CM = 120.0;      // farther than this = free; in between keeps the last state
const float MIN_VALID_CM = 2.0;         // HC-SR04 range limits
const float MAX_VALID_CM = 400.0;

// ---- Timing ----
const unsigned long ECHO_TIMEOUT_US = 30000;  // about 5 m round trip
const int MEDIAN_SAMPLES = 5;
const int MIN_VALID_SAMPLES = 3;              // fewer valid echoes = no reading this cycle
const unsigned long SAMPLE_GAP_MS = 60;       // HC-SR04 measurement cycle is about 60 ms
const unsigned long BAY_GAP_MS = 50;          // Bay 1 echoes fade before Bay 2 pings
const unsigned long CYCLE_MS = 1000;
const int CONFIRM_SAMPLES = 3;                // matching cycles in a row before a change
const unsigned long STALE_AFTER_MS = 30000;   // PLACEHOLDER: from the BAKANTE PDF
const unsigned long HEARTBEAT_MS = 45000;     // registry: at least every 60 s while valid

enum BayState { UNKNOWN, FREE, OCCUPIED };

struct Bay {
  BayState state = UNKNOWN;      // confirmed state
  BayState candidate = UNKNOWN;  // state seen in recent cycles
  int matchCount = 0;
  unsigned long lastValidMs = 0;
  unsigned long lastPublishMs = 0;
};
Bay bays[2];

// Diagnostics from the most recent cycle on each bay
unsigned long lastRawUs[2] = {0, 0};
int lastIdleLevel[2] = {0, 0};
int lastValidCount[2] = {0, 0};

const char* stateName(BayState s) {
  if (s == OCCUPIED) return "OCCUPIED";
  if (s == FREE) return "FREE";
  return "UNKNOWN";
}

// One ping. Returns distance in cm, or -1 if there is no valid echo.
float readDistanceCm(int bay) {
  lastIdleLevel[bay] = digitalRead(ECHO_PIN[bay]);  // should be LOW between pings

  digitalWrite(TRIG_PIN[bay], LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN[bay], HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN[bay], LOW);

  unsigned long echoUs = pulseIn(ECHO_PIN[bay], HIGH, ECHO_TIMEOUT_US);
  lastRawUs[bay] = echoUs;
  if (echoUs == 0) return -1;        // timeout = no echo, NOT an empty bay
  float cm = echoUs * 0.0343 / 2.0;  // speed of sound, halved for the round trip
  if (cm < MIN_VALID_CM || cm > MAX_VALID_CM) return -1;
  return cm;
}

// Median of the valid pings in one cycle. Returns -1 if fewer than
// MIN_VALID_SAMPLES echoes came back, so one stray echo cannot set a reading.
float medianDistanceCm(int bay) {
  float samples[MEDIAN_SAMPLES];
  int n = 0;
  for (int i = 0; i < MEDIAN_SAMPLES; i++) {
    float cm = readDistanceCm(bay);
    if (cm > 0) samples[n++] = cm;
    delay(SAMPLE_GAP_MS);
  }
  lastValidCount[bay] = n;
  if (n < MIN_VALID_SAMPLES) return -1;
  for (int i = 1; i < n; i++) {  // insertion sort
    float key = samples[i];
    int j = i - 1;
    while (j >= 0 && samples[j] > key) {
      samples[j + 1] = samples[j];
      j--;
    }
    samples[j + 1] = key;
  }
  return samples[n / 2];
}

// Hysteresis: a clear distance sets the state; the band between keeps the last one.
// A bay with no confirmed state (at startup or after a timeout) returns UNKNOWN for
// an in-band reading, so it waits for a clear reading.
BayState classify(float cm, BayState previous) {
  if (cm < OCCUPIED_BELOW_CM) return OCCUPIED;
  if (cm > FREE_ABOVE_CM) return FREE;
  return previous;
}

// Dry run: prints the message that would go to the broker.
void publishBay(int bay) {
  Bay &b = bays[bay];
  int value = (b.state == OCCUPIED) ? 1 : 0;
  Serial.printf("PUB %s {\"node\":\"%s\",\"metric\":\"%s\",\"value\":%d,\"unit\":\"bool\",\"uptime\":%lu}\r\n",
                TOPIC_BAY[bay], NODE_ID, METRIC[bay], value, millis() / 1000);
  b.lastPublishMs = millis();
}

void updateBay(int bay, float cm) {
  Bay &b = bays[bay];

  if (cm > 0) {
    b.lastValidMs = millis();
    BayState reading = classify(cm, b.state);

    if (reading == UNKNOWN) {
      // No vote: no confirmed state yet and the reading is inside the band.
      // Breaking the streak keeps "3 in a row" strict.
      b.candidate = UNKNOWN;
      b.matchCount = 0;
    } else {
      if (reading == b.candidate) {
        if (b.matchCount < CONFIRM_SAMPLES) b.matchCount++;  // display stops at 3/3
      } else {
        b.candidate = reading;
        b.matchCount = 1;
      }
      if (b.matchCount >= CONFIRM_SAMPLES && b.candidate != b.state) {
        b.state = b.candidate;
        Serial.printf("CONFIRMED bay=%d -> %s\r\n", bay + 1, stateName(b.state));
        publishBay(bay);
      }
    }

    // Heartbeat only on a cycle with a valid reading, so a silent
    // sensor never gets a repeated FREE or OCCUPIED.
    if (b.state != UNKNOWN && millis() - b.lastPublishMs >= HEARTBEAT_MS) {
      publishBay(bay);
    }
  } else if (b.state != UNKNOWN && millis() - b.lastValidMs > STALE_AFTER_MS) {
    b.state = UNKNOWN;
    b.candidate = UNKNOWN;
    b.matchCount = 0;
    Serial.printf("bay=%d -> UNKNOWN (no valid reading for %lu s; nothing published)\r\n",
                  bay + 1, STALE_AFTER_MS / 1000);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.print("\r\n=== KAHIMTANG n06 dry run (Serial only, no Wi-Fi/MQTT) ===\r\n");
  for (int i = 0; i < 2; i++) {
    pinMode(TRIG_PIN[i], OUTPUT);
    digitalWrite(TRIG_PIN[i], LOW);
    pinMode(ECHO_PIN[i], INPUT);
  }
  Serial.printf("PUB %s {\"status\":\"online\"} (retained; last will = offline)\r\n", TOPIC_STATUS);
}

void loop() {
  unsigned long cycleStart = millis();

for (int i = 0; i < 2; i++) {
    float cm = medianDistanceCm(i);
    updateBay(i, cm);
    Bay &b = bays[i];
    if (cm > 0) {
      Serial.printf("bay=%d median=%.1fcm state=%s candidate=%s (%d/%d)\r\n",
                    i + 1, cm, stateName(b.state), stateName(b.candidate),
                    b.matchCount, CONFIRM_SAMPLES);
    } else {
      Serial.printf("bay=%d no valid echo (%d of %d valid) | ECHO idle=%d, last pulse=%lu us | state=%s\r\n",
                    i + 1, lastValidCount[i], MEDIAN_SAMPLES, lastIdleLevel[i],
                    lastRawUs[i], stateName(b.state));
    }
    delay(BAY_GAP_MS);
  }

  unsigned long elapsed = millis() - cycleStart;
  if (elapsed < CYCLE_MS) delay(CYCLE_MS - elapsed);
}