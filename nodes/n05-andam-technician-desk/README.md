# ANDAM technician desk

The technician desk (`n05`) runs on an ESP32-WROOM-32 DevKit with Arduino and PlatformIO. The second room LED is reserved for `n04`.

- The buzzer is quiet when the desk starts. A signed `beep` command for a new ticket plays one 120 ms pulse. Flash storage preserves the latest accepted ticket ID across reboots.
- Periodic signed `rooms` updates light the room LEDs without sounding the buzzer. The green LED is steady while a snapshot is fresh and blinks while waiting for one.
- Mute silences the current and queued pulses. A lost connection also stops the buzzer and clears its queue.
- The desk sends the number of pulses started since boot on `bca188/blocka/andam/n05/alert-count` when it changes and at least every 45 seconds while connected.

## Hardware Components

| Component | Quantity | Function |
|---|---:|---|
| ESP32-WROOM-32 DevKit | 1 | Main controller |
| Red LED | 2 | Classroom request indicators |
| Green LED | 1 | Network and snapshot status |
| Active buzzer | 1 | Audible notifications |
| Momentary push button | 1 | Buzzer mute control |
| 220Ω resistor | 3 | LED current limiting |
| NPN transistor and base resistor | As required | Buzzer driver |
| Breadboard | 1 | Circuit prototyping |
| Jumper wires | As required | Electrical connections |
| USB cable | 1 | Programming and power |

## GPIO Pin Assignments

| ESP32 GPIO | Component | Pin Mode | Purpose |
|---|---|---|---|
| GPIO 32 | Red LED 1 | OUTPUT | N03 ICT3D indicator |
| GPIO 33 | Red LED 2 | OUTPUT | N04 reserved classroom indicator |
| GPIO 26 | Green LED | OUTPUT | System status indicator |
| GPIO 27 | Active buzzer | OUTPUT | Audible notification |
| GPIO 4 | Mute button | INPUT_PULLUP | Silence the current alert |
| GND | Common ground | — | Shared electrical reference |

**Important:** GPIO 33 is reserved for N04. N04 is not currently included in the two-node implementation.

## Wiring Instructions

### N03 Classroom Indicator – Red LED

| Connection | Destination |
|---|---|
| ESP32 GPIO 32 | 220Ω resistor |
| Resistor output | Red LED anode (+) |
| Red LED cathode (-) | GND |

### N04 Classroom Indicator – Red LED (Reserved)

| Connection | Destination |
|---|---|
| ESP32 GPIO 33 | 220Ω resistor |
| Resistor output | Red LED anode (+) |
| Red LED cathode (-) | GND |

This indicator is reserved for a future N04 classroom node.

### System Status – Green LED

| Connection | Destination |
|---|---|
| ESP32 GPIO 26 | 220Ω resistor |
| Resistor output | Green LED anode (+) |
| Green LED cathode (-) | GND |

### Active Buzzer

| Buzzer Terminal | Connection |
|---|---|
| Positive (+) | GPIO 27, through a suitable driver if needed |
| Negative (-) | GND |

**Electrical safety:** Check the buzzer's rated operating voltage and current. Do not connect a buzzer that exceeds the ESP32 GPIO output capability directly to GPIO 27. Use an appropriate transistor driver and suitable supply when required. Connect grounds together.

### Mute Button

| Button Terminal | Connection |
|---|---|
| Terminal 1 | GPIO 4 |
| Terminal 2 | GND |

The firmware configures GPIO 4 as `INPUT_PULLUP`, so the button reads HIGH when released and LOW when pressed.

### Wiring Summary

```text
ESP32-WROOM-32
│
├── GPIO 32 ── 220Ω ── Red LED (N03) ── GND
│
├── GPIO 33 ── 220Ω ── Red LED (N04 reserved) ── GND
│
├── GPIO 26 ── 220Ω ── Green LED ── GND
│
├── GPIO 27 ── Buzzer Driver / Active Buzzer
│
├── GPIO 4  ── Mute Button ── GND
│
└── GND ───── Common Ground
```

**Note:** This is a connection reference, not a physical breadboard layout. Confirm component polarity and the actual circuit before powering the board.

## Build and test

1. Open this station as a PlatformIO project.
2. Copy [`include/secrets.example.h`](include/secrets.example.h) to `include/secrets.h`. Set the node's broker credentials, trusted CA and the matching private key in the [ANDAM service](../../backend/README.md).
3. Build and upload the sketch. Set Serial Monitor to 115200 baud.
4. With the service running, [test one ICT3D press and one desk reconnect](../../TESTING.md#andam-bench-test).

See the [data registry](../../DATA_REGISTRY.md#andam-message-formats) for the message formats. Earlier V3 tests used the team's local Mosquitto setup. The updated firmware needs a test on the physical boards with the assigned broker.
