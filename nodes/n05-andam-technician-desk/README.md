# ANDAM V3 – N05 Technician Desk

**Project:** ANDAM – Smart University Assistance and Notification System  
**Node ID:** `n05`  
**Device:** Technician Desk Notification Unit  
**Microcontroller:** ESP32-WROOM-32 DevKit  
**Firmware:** Arduino C++  
**Development Environment:** Visual Studio Code + PlatformIO  
**Communication:** MQTT over Wi-Fi  
**Broker:** Eclipse Mosquitto  
**Backend:** Python  
**Development Status:** Hardware and communication integration testing

---

## 1. Project Overview

The N05 Technician Desk is the technician-side notification unit of the ANDAM system.

Its primary function is to receive classroom assistance-request updates from the Python backend and notify the technician through physical indicators.

When a classroom submits a help request, the backend processes and stores the ticket before publishing an updated open-ticket snapshot to N05.

The N05 ESP32 processes the received snapshot and activates the corresponding classroom indicator LED. An audible buzzer provides additional notification when a new request appears.

The technician handles ticket acknowledgement through the backend interface. N05 also periodically sends heartbeat messages so the backend can monitor its network availability.

The current implementation focuses on two nodes:

| Node | Description | Status |
|---|---|---|
| N03 | Classroom 1 Help Station | Firmware integration pending |
| N05 | Technician Desk | Flashed; Wi-Fi and NTP verified |

A second room indicator is reserved for a future N04 classroom node.

## 2. System Architecture

```text
       N03 CLASSROOM
       ESP32-WROOM-32
              |
        HELP REQUEST
              |
              v
       MQTT BROKER
        MOSQUITTO
              |
              v
       PYTHON BACKEND
              |
       Ticket Processing
       Database Storage
              |
              v
       MQTT BROKER
              |
              v
       N05 TECHNICIAN DESK
       ESP32-WROOM-32
              |
       +------+------+
       |      |      |
      LED   BUZZER  STATUS
       |      |      |
       +------+------+
              |
         TECHNICIAN
              |
       Backend Acknowledgement
              |
              v
          N03 CLASSROOM
```

### Communication Responsibilities

**N03 – Classroom Node**
- Submits help requests.
- Sends resolution or cancellation events.
- Receives ticket confirmation and technician acknowledgement.

**Python Backend**
- Receives and validates node messages.
- Records help requests.
- Prevents duplicate ticket creation.
- Publishes updated open-ticket snapshots.
- Processes technician acknowledgement.
- Monitors node heartbeats.

**N05 – Technician Node**
- Receives open-ticket snapshots.
- Identifies classrooms with active tickets.
- Controls room indicator LEDs.
- Activates the buzzer for new requests.
- Provides recurring audible reminders.
- Sends periodic heartbeat messages.

## 3. Hardware Components

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

## 4. GPIO Pin Assignments

| ESP32 GPIO | Component | Pin Mode | Purpose |
|---|---|---|---|
| GPIO 32 | Red LED 1 | OUTPUT | N03 Classroom 1 indicator |
| GPIO 33 | Red LED 2 | OUTPUT | N04 reserved classroom indicator |
| GPIO 26 | Green LED | OUTPUT | System status indicator |
| GPIO 27 | Active buzzer | OUTPUT | Audible notification |
| GPIO 4 | Mute button | INPUT_PULLUP | Silence recurring reminders |
| GND | Common ground | — | Shared electrical reference |

**Important:** GPIO 33 is reserved for N04. N04 is not currently included in the two-node implementation.

## 5. Wiring Instructions

### 5.1 N03 Classroom Indicator – Red LED

| Connection | Destination |
|---|---|
| ESP32 GPIO 32 | 220Ω resistor |
| Resistor output | Red LED anode (+) |
| Red LED cathode (-) | GND |

### 5.2 N04 Classroom Indicator – Red LED (Reserved)

| Connection | Destination |
|---|---|
| ESP32 GPIO 33 | 220Ω resistor |
| Resistor output | Red LED anode (+) |
| Red LED cathode (-) | GND |

This indicator is reserved for a future N04 classroom node.

### 5.3 System Status – Green LED

| Connection | Destination |
|---|---|
| ESP32 GPIO 26 | 220Ω resistor |
| Resistor output | Green LED anode (+) |
| Green LED cathode (-) | GND |

### 5.4 Active Buzzer

| Buzzer Terminal | Connection |
|---|---|
| Positive (+) | GPIO 27, through a suitable driver if needed |
| Negative (-) | GND |

**Electrical safety:** Check the buzzer's rated operating voltage and current. Do not connect a buzzer that exceeds the ESP32 GPIO output capability directly to GPIO 27. Use an appropriate transistor driver and suitable supply when required. Connect grounds together.

### 5.5 Mute Button

| Button Terminal | Connection |
|---|---|
| Terminal 1 | GPIO 4 |
| Terminal 2 | GND |

The firmware configures GPIO 4 as `INPUT_PULLUP`, so the button reads HIGH when released and LOW when pressed.

### 5.6 Wiring Summary

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

## 6. Intended Operating Logic

### 6.1 Startup

1. Initialize GPIO pins.
2. Initialize HMAC security and persistent sequence counters.
3. Connect to the configured Wi-Fi network.
4. Synchronize system time using NTP.
5. Attempt MQTT broker connection.
6. Produce one short startup beep.

### 6.2 New Help Request

When N05 receives a valid snapshot containing a newly opened classroom ticket:

1. Validate the sender node ID.
2. Validate the snapshot state.
3. Check the message timestamp.
4. Check the sequence number.
5. Verify the HMAC-SHA256 signature.
6. Identify the classroom with an open ticket.
7. Illuminate the corresponding room indicator.
8. Produce three audible beeps.

### 6.3 Recurring Reminder

When an open ticket remains active:

- The corresponding room LED stays illuminated.
- Three reminder beeps are scheduled every 60 seconds.
- The mute button suppresses recurring audible reminders.
- A newly appearing classroom request clears the mute state and triggers a new notification.

### 6.4 No Open Tickets

When there are no active classroom tickets:

- Both room LEDs turn off.
- The green status LED remains solid when a fresh snapshot is available.
- Recurring audible reminders stop.

### 6.5 Connection or Snapshot Unavailable

When MQTT is disconnected or the last valid snapshot becomes stale:

- Room LEDs turn off.
- The green status LED blinks slowly.
- The device does not treat an outdated snapshot as current.

These are the intended behaviors implemented in the current N05 firmware and still require complete hardware and integration verification.

## 7. MQTT Communication

N05 communicates with the Python backend through Mosquitto.

### 7.1 MQTT Topics

| Topic | Direction | Purpose |
|---|---|---|
| `sentra/andam/n05/status` | N05 → Backend | Online/offline status |
| `sentra/andam/n05/heartbeat` | N05 → Backend | Periodic heartbeat |
| `sentra/andam/n05/alert` | Backend → N05 | Open-ticket snapshots |

### 7.2 Snapshot Payload

Example:

```json
{
  "node": "n05",
  "state": "open_tickets",
  "rooms": "n03",
  "seq": 150,
  "ts": 1763500000,
  "sig": "HMAC_SHA256_SIGNATURE"
}
```

The `rooms` field identifies which classroom nodes currently have open tickets.

Possible values:

| Value | Meaning |
|---|---|
| `n03` | N03 has an open ticket |
| `n04` | N04 has an open ticket (future node) |
| `n03,n04` | Both classrooms have open tickets |
| Empty string | No open tickets |

### 7.3 Heartbeat Payload

Example:

```json
{
  "node": "n05",
  "uptime_s": 3600,
  "rssi_dbm": -55,
  "seq": 151,
  "ts": 1763500030,
  "sig": "HMAC_SHA256_SIGNATURE"
}
```

Heartbeat interval: 30 seconds.

The backend uses heartbeat messages to monitor node availability.

All JSON examples are illustrative and do not represent captured production messages.

## 8. Security and Reliability

ANDAM V3 uses HMAC-SHA256 message authentication and message-validation mechanisms.

The current design includes:

- Unique HMAC key per ESP32 node.
- HMAC-SHA256 message signatures.
- Timestamp validation.
- Sequence-number validation.
- Constant-time signature comparison.
- Persistent outgoing sequence counters.
- MQTT access-control lists.
- Retained online/offline status messages.
- Duplicate request detection in the backend.
- Snapshot freshness checking.

### Credential Management

The local `include/secrets.h` file contains private configuration values.

It must not be committed to the public repository.

Required configuration includes:

- Wi-Fi SSID and password.
- MQTT broker IP address.
- MQTT port.
- N05 MQTT username and password.
- N05 HMAC key.
- TLS configuration and CA certificate when TLS is enabled.

The MQTT username for this device must be `n05`, not `backend`.

## 9. Software Requirements

| Software / Library | Purpose |
|---|---|
| Visual Studio Code | Development environment |
| PlatformIO | Firmware build and upload |
| ESP32 Arduino Framework | Microcontroller framework |
| ArduinoJson 7 | JSON serialization and parsing |
| PubSubClient 2.8 | MQTT client |
| Eclipse Mosquitto | MQTT broker |
| Python 3 | Backend execution |
| SQLite | Local backend database |
| HMAC-SHA256 | Message authentication |

### PlatformIO Dependencies

```ini
lib_deps =
    knolleary/PubSubClient@^2.8
    bblanchon/ArduinoJson@^7.4.2
```

## 10. Installation and Execution

### Step 1 – Open the Project

Open this directory in VS Code:

`nodes/n05-andam-technician-desk`

### Step 2 – Configure Secrets

Create the local `include/secrets.h` file and configure the Wi-Fi, MQTT, HMAC, and TLS settings.

The N05 HMAC key must match the `n05_key` configured in the backend.

### Step 3 – Build Firmware

```powershell
pio run
```

### Step 4 – Upload Firmware

```powershell
pio run --target upload
```

### Step 5 – Open Serial Monitor

```powershell
pio device monitor
```

Serial baud rate:

```text
115200
```

### Step 6 – Start Mosquitto

The local development broker configuration is located at:

`C:\mosquitto-test\mosquitto.conf`

The current local broker listens on TCP port `1884`.

Check the broker:

```powershell
netstat -ano | findstr :1884
```

Expected listener:

```text
TCP    0.0.0.0:1884    0.0.0.0:0    LISTENING
```

Do not start another broker instance if port 1884 is already occupied.

### Step 7 – Start Python Backend

```powershell
cd C:\andam-node\backend
py backend.py
```

Expected startup output:

```text
ANDAM V3 LOCAL TEST BACKEND
Broker: 127.0.0.1:1884
Database: andam_test.db
[MQTT] Connected to 127.0.0.1:1884
[MQTT] Subscribed to ANDAM request, resolve, heartbeat and status topics
```

The ESP32 must use the laptop's LAN IPv4 address as `BROKER_HOST`, not `127.0.0.1`.

## 11. Development Progress and Verification Status

**Last documented progress: November 21, 2026**

N05 has been successfully compiled, flashed, booted, connected to Wi-Fi, and synchronized with NTP.

However, its MQTT connection and full physical notification workflow are not yet verified.

### 11.1 Verified Progress

| Task | Status | Evidence |
|---|---|---|
| N05 firmware compilation | Verified | Clean PlatformIO build reported |
| Firmware upload | Verified | ESP32 successfully boots |
| ESP32 startup | Verified | Power-on boot output received |
| Wi-Fi connection | Verified | Serial Monitor reports connection |
| IP assignment | Verified | `192.168.1.22` during testing |
| NTP synchronization | Verified | Valid timestamp received |
| Mosquitto startup | Verified | Broker process running |
| Broker TCP listener | Verified | Port 1884 listening |
| Python backend startup | Verified | Backend connected to Mosquitto |
| Backend snapshot publishing | Verified | Increasing snapshot sequence numbers |

### 11.2 Pending Verification

| Task | Status | Required Action |
|---|---|---|
