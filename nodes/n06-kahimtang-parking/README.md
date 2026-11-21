# KAHIMTANG · Parking bays

- **Proposed node:** `n06`
- **Owner or co-owner:** Pending team assignment
- **Field location:** Two campus parking bays, actual bay IDs pending
- **Hardware:** Two HC-SR04 sensors, each with an ECHO voltage divider

See the [data registry](../../DATA_REGISTRY.md) for the topics, units, and reporting interval.

## First test

Test each bay with and without a vehicle. Verify that the board checks the sensors one at a time and shows Unknown when a reading is missing or stale.

Log the actual result in [TESTING.md](../../TESTING.md). Store private Wi‑Fi and class broker credentials in the ignored `secrets.h` file when the sketch is ready.


## Board and pin map

- **Board:** ESP32 DevKit V1 (30-pin, DOIT), confirm against the label on the board
- **Bay 1:** TRIG GPIO 26, ECHO GPIO 27 through the 2.2 kΩ / 3.3 kΩ divider
- **Bay 2:** TRIG GPIO 32, ECHO GPIO 33 through the 2.2 kΩ / 3.3 kΩ divider
- **Status LED (simulation only):** GPIO 25 through a 220 Ω resistor

## Status

-- **Wokwi simulation:** passed (Serial dry run: T04, T05, T10)
- **Physical device, class broker, dashboard:** pending
- **Owner and bay labels:** pending team assignment
