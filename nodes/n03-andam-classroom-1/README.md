# ANDAM classroom help station

The ICT3D station (`n03`) runs on an ESP32-WROOM-32 DevKit with Arduino and PlatformIO. HELP opens a ticket. RESOLVE closes the saved ticket.

- The red LED blinks while a connected station waits for the backend to save the ticket. A signed `received` reply lights the green LED for one second. The red LED stays on while the ticket is open.
- The station keeps the same request count in flash and retries every five seconds until the backend confirms the action, including after a restart.
- RESOLVE works after the ticket is saved. The red LED stays on until the signed `resolved` reply. The LEDs are off while disconnected.
- A count heartbeat is sent every 45 seconds while idle or while a ticket is already saved. Heartbeats do not open tickets.

## Hardware Components

| Component | Quantity | Purpose |
|---|---:|---|
| ESP32-WROOM-32 DevKit | 1 | Main microcontroller |
| Momentary push button | 2 | HELP and RESOLVE inputs |
| Red LED | 1 | Request feedback indicator |
| Green LED | 1 | Confirmation/status feedback |
| 220Ω resistor | 2 | LED current limiting |
| Breadboard | 1 | Circuit prototyping |
| Jumper wires | As needed | Electrical connections |
| USB cable | 1 | Programming and power |

## GPIO Pin Assignments

| ESP32 GPIO | Component | Configuration |
|---|---|---|
| GPIO 4 | HELP button | INPUT_PULLUP |
| GPIO 5 | RESOLVE button | INPUT_PULLUP |
| GPIO 25 | Red LED | OUTPUT |
| GPIO 26 | Green LED | OUTPUT |
| GND | Button and LED ground | Common ground |

### Wiring Instructions

**HELP Button – GPIO 4**
- Connect one terminal of the momentary button to GPIO 4.
- Connect the opposite terminal to GND.
- Configure the pin as `INPUT_PULLUP`.

**RESOLVE Button – GPIO 5**
- Connect one terminal of the momentary button to GPIO 5.
- Connect the opposite terminal to GND.
- Configure the pin as `INPUT_PULLUP`.

**Red LED – GPIO 25**
- Connect GPIO 25 to a 220Ω resistor.
- Connect the resistor to the LED anode (long leg).
- Connect the LED cathode (short leg) to GND.

**Green LED – GPIO 26**
- Connect GPIO 26 to a separate 220Ω resistor.
- Connect the resistor to the LED anode (long leg).
- Connect the LED cathode (short leg) to GND.

**Important:** GPIO 5 is a boot-strapping pin on the ESP32. Avoid holding the RESOLVE button while powering on or resetting the board.

All components must share a common ground.

## Build and test

1. Open this station as a PlatformIO project.
2. Copy [`include/secrets.example.h`](include/secrets.example.h) to `include/secrets.h`. Set the node's broker credentials, trusted CA and the matching private key in the [ANDAM service](../../backend/README.md).
3. Build and upload the sketch. Set Serial Monitor to 115200 baud.
4. With the service running, [test one ICT3D press and one desk reconnect](../../TESTING.md#andam-bench-test).

See the [data registry](../../DATA_REGISTRY.md#andam-message-formats) for the message formats. Earlier V3 tests used the team's local Mosquitto setup. The updated firmware needs a test on the physical boards with the assigned broker.
