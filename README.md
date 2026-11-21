![SENTRA banner](assets/readme/sentra-header.png)

# SENTRA

We’re building SENTRA for our BCA188 Block A project at MSU-IIT. In our first campus field test, we’ll monitor conditions in one classroom, give instructors in two classrooms a way to ask for technician help, and show whether two parking bays are available.

![ESP32](https://img.shields.io/badge/ESP32-171717?style=flat-square&logo=espressif&logoColor=white)
![MQTT](https://img.shields.io/badge/MQTT-171717?style=flat-square)
![FastAPI](https://img.shields.io/badge/FastAPI-171717?style=flat-square&logo=fastapi&logoColor=white)
![React](https://img.shields.io/badge/React-171717?style=flat-square&logo=react&logoColor=white)

[Subsystems](#what-sentra-does) · [Architecture](#how-data-reaches-the-dashboard) · [Node plan](#six-esp32-nodes) · [Setup](#getting-started) · [Testing](#field-test) · [Team](#team-contributions)

## What SENTRA does

| Subsystem | What it does |
| --- | --- |
| <img src="assets/readme/hayag.svg" width="24" height="24" alt=""> **HAYAG** | Tracks temperature, humidity, light at the window and desk, and recent movement in one classroom. |
| <img src="assets/readme/andam.svg" width="24" height="24" alt=""> **ANDAM** | Gives instructors in two classrooms a help button. When SENTRA saves a ticket with the room name, a short beep alerts the technician. |
| <img src="assets/readme/kahimtang.svg" width="24" height="24" alt=""> **KAHIMTANG** | Lets authorized faculty and staff check two monitored parking bays. A missing or stale reading shows **Unknown**. |

## How data reaches the dashboard

ESP32 nodes publish registered readings and events to the instructor’s MQTT broker. A Python service on campus validates the messages and makes them available to the local React dashboard. The planned off-campus view uses Supabase for stored readings and live updates, with Vercel hosting the web app. The campus service needs internet access to send those updates.

```mermaid
flowchart LR
    A["ESP32 nodes"] -->|MQTT| B["Instructor’s MQTT broker"]
    B --> C["Campus service · Paho MQTT and FastAPI"]
    C --> D["Campus dashboard"]
    C -.->|Planned cloud sync| E["Supabase"]
    E -.-> F["Off-campus dashboard · Vercel"]
```

Keep class broker credentials in local configuration. The campus service connects to the broker using Block A’s assigned topics under `bca188/blocka/#`.

## Six ESP32 nodes

| Board | Station | Connected parts |
| --- | --- | --- |
| 1 | HAYAG climate and motion | SHT30-D temperature and humidity sensor and HC-SR501 motion sensor |
| 2 | HAYAG lighting | Two BH1750 light sensors at separate measurement points and a small LED for the field test |
| 3 | ANDAM Classroom 1 | Momentary help button and feedback LED |
| 4 | ANDAM Classroom 2 | Momentary help button and feedback LED |
| 5 | ANDAM technician desk | Three-pin active buzzer module |
| 6 | KAHIMTANG parking | Two HC-SR04 ultrasonic sensors, one per bay, each with an ECHO voltage divider |

Board numbers identify our working hardware layout. Final MQTT node IDs, locations, and owners belong in the team’s data registry.

## Getting started

Clone the repository to get a local copy.

```sh
git clone https://github.com/xavieryyyyyy/bca188-blocka-smart-university.git
cd bca188-blocka-smart-university
```

This repository is in setup. Before the first MQTT test, assign a node owner and register the node’s readings and topics in `DATA_REGISTRY.md`. Once the first sketch is ready, test it in Serial Monitor at 115200 baud, confirm its messages in MQTT Explorer, then check them on the dashboard. Use `secrets.h` for local ESP32 credentials and `.env` for local service settings.

## Field test

Start with one ANDAM classroom station. Check that one button press creates one ticket for the correct room and causes one beep at the technician desk. Then check the HAYAG readings in the classroom and test each KAHIMTANG bay with and without a vehicle. During the network test, verify that each offline or stale reading is labeled clearly and that updates resume after reconnection.

Record what happened in `TESTING.md`. Use the data registry for approved measurements and node owners, `SECURITY.md` for access and privacy controls, and `slides/` for the final presentation PDF as those deliverables are prepared.

## Team contributions

SENTRA is the work of BCA188 Block A’s 19 members. Each student owns or co-owns a node or dashboard feature and records their work through their own Git commits. The team will record assignments alongside the field-test plan.

## Credits and AI use

Our project follows MSU-IIT’s BCA188 Smart University System handout and Lab 16 integration guide. The README uses [Inter](https://rsms.me/inter/) and [Lucide icons](https://github.com/lucide-icons/lucide). OpenAI ChatGPT helped with planning, the README mockup, and documentation.
