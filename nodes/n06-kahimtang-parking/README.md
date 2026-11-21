# KAHIMTANG · Parking bays

- **Proposed node:** `n06`
- **Owner or co-owner:** Pending team assignment
- **Field location:** Two campus parking bays, actual bay IDs pending
- **Hardware:** Two HC-SR04 sensors, each with an ECHO voltage divider

See the [data registry](../../DATA_REGISTRY.md) for the topics, units, and reporting interval.

## First test

Test each bay with and without a vehicle. Verify that the board checks the sensors one at a time and shows Unknown when a reading is missing or stale.

Log the actual result in [TESTING.md](../../TESTING.md). Store private Wi‑Fi and class broker credentials in the ignored `secrets.h` file when the sketch is ready.
