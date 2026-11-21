# ANDAM – N03 Classroom Help Station

**Project:** ANDAM – Smart University Assistance and Notification System  
**Node ID:** `n03`  
**Node Name:** Classroom 1 Help Station  
**Microcontroller:** ESP32-WROOM-32  
**Firmware:** ESP32 Arduino C++  
**Development Environment:** Visual Studio Code + PlatformIO  
**Communication Protocol:** MQTT  
**Project Status:** Under Development

---

## 1. Project Overview

The ANDAM Classroom Help Station is an IoT-based assistance system designed to allow students or classroom users to request technical assistance without having to physically visit the technician's desk.

The N03 Classroom Help Station serves as the request-input device for Classroom 1. When a user presses the HELP button, the ESP32 generates a request identifier and transmits an authenticated request to the backend through an MQTT broker.

The backend processes and records the request before notifying the N05 Technician Desk. Once the technician acknowledges or resolves the request, the system communicates the corresponding status back to the classroom node.

The system is designed to improve communication between classrooms and technical personnel while providing a structured and traceable assistance-request workflow.

## 2. System Architecture

ANDAM V3 currently follows a two-node architecture:

| Node | Designation | Responsibility |
|---|---|---|
| N03 | Classroom 1 Help Station | Submits help requests and resolution/cancellation events |
| N05 | Technician Desk | Receives room notifications and handles technician acknowledgement |

### Communication Flow

```text
       N03 CLASSROOM
      ESP32-WROOM-32
             |
       HELP / RESOLVE
             |
             v
      MQTT BROKER
        MOSQUITTO
             |
             v
       PYTHON BACKEND
             |
       Ticket Processing
       and Database Storage
             |
             v
      MQTT BROKER
             |
             v
      N05 TECHNICIAN DESK
        ESP32-WROOM-32
             |
       LED / BUZZER ALERT
             |
       Technician Action
             |
             v
       BACKEND ACKNOWLEDGEMENT
             |
             v
       N03 CLASSROOM
```

## 3. Hardware Components

| Component | Quantity | Purpose |
|---|---:|---|
| ESP32-WROOM-32 DevKit | 1 | Main microcontroller |
| Momentary push button | 2 | HELP and RESOLVE/CANCEL inputs |
| Red LED | 1 | Request feedback indicator |
| Green LED | 1 | Confirmation/status feedback |
| 220Ω resistor | 2 | LED current limiting |
| Breadboard | 1 | Circuit prototyping |
| Jumper wires | As needed | Electrical connections |
| USB cable | 1 | Programming and power |

## 4. GPIO Pin Assignments

| ESP32 GPIO | Component | Configuration |
|---|---|---|
| GPIO 4 | HELP button | INPUT_PULLUP |
| GPIO 5 | RESOLVE/CANCEL button | INPUT_PULLUP |
| GPIO 25 | Red LED | OUTPUT |
| GPIO 26 | Green LED | OUTPUT |
| GND | Button and LED ground | Common ground |

### Wiring Instructions

**HELP Button – GPIO 4**
- Connect one terminal of the momentary button to GPIO 4.
- Connect the opposite terminal to GND.
- Configure the pin as `INPUT_PULLUP`.

**RESOLVE/CANCEL Button – GPIO 5**
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

**Important:** GPIO 5 is a boot-strapping pin on the ESP32. Avoid holding the RESOLVE/CANCEL button while powering on or resetting the board.

All components must share a common ground.

## 5. Intended Operating Behavior

### HELP Request

1. The user presses the HELP button.
2. The ESP32 detects and debounces the button input.
3. A new persistent request ID is generated.
4. The node constructs an authenticated MQTT request.
5. The request is transmitted to the backend.
6. The backend validates and records the request.
7. The backend sends a `ticket_saved` confirmation.
8. The classroom node processes the confirmation.

### Technician Acknowledgement

1. The technician receives a new request at the N05 desk.
2. The technician acknowledges the request.
3. The backend processes the acknowledgement.
4. An authenticated acknowledgement command is published to N03.
5. N03 verifies the command and checks that the request ID matches its active request.

### Resolve / Cancel

The RESOLVE/CANCEL button sends the corresponding event to the backend.

Supported events:

- `resolved`
- `cancelled`

These workflows describe the intended V3 behavior and remain subject to hardware integration testing.

## 6. MQTT Communication

The N03 node communicates with the Python backend using the following MQTT topics.

| MQTT Topic | Direction | Purpose |
|---|---|---|
| `sentra/andam/n03/request` | N03 → Backend | Submit help request |
| `sentra/andam/n03/resolve` | N03 → Backend | Submit resolution or cancellation |
| `sentra/andam/n03/status` | N03 → Backend | Online/offline status |
| `sentra/andam/n03/heartbeat` | N03 → Backend | Periodic node heartbeat |
| `sentra/andam/n03/cmd` | Backend → N03 | Ticket confirmation and acknowledgement |

### Message Format

Example help-request payload:

```json
{
  "node": "n03",
  "event": "help_request",
  "req": 41,
  "seq": 120,
  "ts": 1763500000,
  "sig": "HMAC_SHA256_SIGNATURE"
}
```

Fields:

| Field | Description |
|---|---|
| `node` | Unique node identifier |
| `event` | Event type |
| `req` | Persistent request identifier |
| `seq` | Message sequence number |
| `ts` | UTC Unix timestamp |
| `sig` | HMAC-SHA256 message signature |

The values above are illustrative and are not actual credentials or test results.

## 7. Security and Reliability

ANDAM V3 incorporates message authentication and replay-protection mechanisms.

Security and reliability features in the design include:

- HMAC-SHA256 message authentication.
- Unique HMAC key for each ESP32 node.
- Timestamp validation.
- Message sequence-number validation.
- Persistent request IDs.
- Duplicate-request detection by the backend.
- MQTT access-control restrictions.
- Retry of unconfirmed requests using the same request ID.
- Retained MQTT status messages for node availability.

### Credential Management

Private configuration values must remain outside the public repository.

The local `include/secrets.h` file should contain the required configuration, including:

- Wi-Fi SSID and password.
- MQTT broker address and port.
- N03 MQTT username and password.
- N03 HMAC key.
- TLS certificate configuration, when TLS is enabled.

**Never commit actual Wi-Fi credentials, MQTT passwords, HMAC keys, or private certificates to GitHub.**

## 8. Software and Development Requirements

| Requirement | Technology |
|---|---|
| IDE | Visual Studio Code |
| Firmware build system | PlatformIO |
| Programming language | C++ |
| Microcontroller framework | Arduino ESP32 |
| MQTT client library | PubSubClient |
| JSON processing | ArduinoJson |
| MQTT broker | Eclipse Mosquitto |
| Backend | Python |
| Database | SQLite (local development backend) |
| Message authentication | HMAC-SHA256 |

### Dependencies

The PlatformIO project uses:

```ini
lib_deps =
    knolleary/PubSubClient@^2.8
    bblanchon/ArduinoJson@^7.4.2
```

## 9. Installation and Setup

### Step 1 – Open the Project

Open the `n03-andam-classroom-1` directory in Visual Studio Code.

### Step 2 – Configure Credentials

Create the local `include/secrets.h` file using the configuration definitions expected by the firmware.

Ensure the N03 credentials and HMAC key match the corresponding backend configuration.

### Step 3 – Build the Firmware

Open the PlatformIO terminal:

```powershell
pio run
```

### Step 4 – Upload to ESP32

Connect the ESP32 through USB and run:

```powershell
pio run --target upload
```

### Step 5 – Open Serial Monitor

```powershell
pio device monitor
```

Serial Monitor baud rate:

```text
115200
```

### Step 6 – Start the Backend and MQTT Broker

The Mosquitto broker and Python backend must be running and configured with matching node credentials and topic permissions.

For local development, the MQTT broker uses port `1884`.

The ESP32 must use the laptop's LAN IPv4 address as its broker host, not `127.0.0.1`.

## 10. Current Development Status

**Last documented development status: November 21, 2026**

The project is currently in the firmware development and hardware integration stage.

### Implemented in the V3 design

- [x] Two-node architecture defined (N03 and N05).
- [x] N03 GPIO pin assignments defined.
- [x] MQTT topic structure established.
- [x] Request and resolution message formats defined.
- [x] HMAC-SHA256 authentication design.
- [x] Persistent request ID design.
- [x] Message sequence-number design.
- [x] Backend ticket-processing workflow implemented.
- [x] Duplicate-request handling implemented in the backend.
- [x] N05 firmware project prepared.

### Verified components

| Component | Status | Notes |
|---|---|---|
| Mosquitto broker | Verified running | Listening on TCP port 1884 |
| Python backend | Verified running | Successfully publishes N05 snapshots |
| Backend-to-broker connection | Verified | Local connection established |
| N05 firmware upload | Verified | ESP32 successfully booted |
| N05 Wi-Fi | Verified | Connected and assigned an IP address |
| N05 NTP synchronization | Verified | Valid UTC time obtained |
| N05 MQTT connection | Pending | Successful ESP32 MQTT connection not yet confirmed |
| N03 firmware | Pending | Not yet flashed or hardware-tested |

### N03 testing checklist

- [ ] Verify PlatformIO compilation.
- [ ] Upload firmware to the N03 ESP32.
- [ ] Verify Wi-Fi connectivity.
- [ ] Verify NTP time synchronization.
- [ ] Verify MQTT authentication.
- [ ] Verify HELP button input.
- [ ] Verify request ID generation.
- [ ] Verify request transmission.
- [ ] Verify backend ticket creation.
- [ ] Verify `ticket_saved` confirmation.
- [ ] Verify RESOLVE/CANCEL button operation.
- [ ] Verify technician acknowledgement reaches N03.
- [ ] Verify LED feedback behavior.
- [ ] Verify duplicate-request handling.
- [ ] Complete end-to-end integration testing with N05.

**Status note:** The N03 firmware has not yet been confirmed working on physical hardware. The N05 Wi-Fi and NTP functions are verified, but N05 MQTT delivery remains unconfirmed. Therefore, the complete two-node communication workflow is not yet considered operational.

## 11. Known Development Issues

1. N05 MQTT connectivity and authentication still require verification.
2. N03 hardware and firmware integration have not yet been tested.
3. End-to-end request delivery and acknowledgement remain unverified.
4. Production deployment requires properly configured network security and credentials.

## 12. Future Development Tasks

- Complete N03 firmware compilation and deployment.
- Establish reliable MQTT connectivity for both nodes.
- Validate the complete help-request workflow.
- Verify technician acknowledgement and request resolution.
- Conduct communication-loss and reconnection testing.
- Verify request persistence and duplicate prevention.
- Perform security and replay-protection tests.
- Conduct classroom and technician-desk field testing.

## 13. Repository Structure

```text
n03-andam-classroom-1/
│
├── include/
│   ├── AndamNetwork.h
│   ├── AndamSecurity.h
│   ├── Config.h
│   ├── MqttBase.h
│   ├── NodeConfig.h
│   └── secrets.h              # Local only; excluded from Git
│
├── src/
│   └── main.cpp
│
├── platformio.ini
├── README.md
└── TESTING.md
```

The actual directory contents may vary as firmware development progresses.

## 14. Project Notes

N03 is the first classroom help station in the ANDAM V3 two-node implementation.

The system is being developed incrementally, with individual hardware, firmware, communication, and backend functions verified before full integration.

All verification results should be recorded in `TESTING.md`. Features that have only been designed or implemented in code must not be reported as successfully tested until their corresponding tests have been completed.

---

**ANDAM – Smart University Assistance and Notification System**  
*N03 Classroom Help Station | Firmware Development Documentation*
