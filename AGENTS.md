# Working on SENTRA

SENTRA is MSU-IIT BCA188 Block A’s Smart University project. The first campus field test uses six ESP32s across one monitored classroom, two classrooms with help buttons, a technician desk, and two parking bays.

If you are a teammate or helping one with an AI tool, start with the [project overview](README.md), the [station list](nodes/), and the [data registry](DATA_REGISTRY.md). Find the teammate’s assigned station and branch before proposing changes. Use the [contribution guide](CONTRIBUTING.md) for commits and pull requests.

## Ownership and handoff

| Owner | Where to work | What to show before integration |
| --- | --- | --- |
| **HAYAG** | [Climate and motion](nodes/n01-hayag-climate/) and [lighting](nodes/n02-hayag-lighting/) | Temperature, humidity, and recent-motion readings. Cover each BH1750 separately and show which lux reading changed. Include the model LED test. |
| **ANDAM** | [Classroom 1](nodes/n03-andam-classroom-1/), [Classroom 2](nodes/n04-andam-classroom-2/), and [technician desk](nodes/n05-andam-technician-desk/) | Each button records one press for its own classroom. Bench-test the feedback LEDs and buzzer. During integration, show that saving a new ticket causes one desk beep and that reconnecting keeps old tickets quiet. |
| **KAHIMTANG** | [Two-bay parking station](nodes/n06-kahimtang-parking/) | Test each bay empty and occupied, then test both sensors together. Check that a missing or stale reading becomes **Unknown**. |
| **Yestin Paraguya, project lead** | [Dashboard](dashboard/) | Own the dashboard code and coordinate how verified readings, node status, and ANDAM tickets appear. Subsystem teams bring their data and test results. |

Agree on shared interfaces with the project lead and the people working in [backend/](backend/) before changing code another team depends on. Subsystem teams can bench-test and verify their approved MQTT messages while dashboard work is in progress.

## Agree on the data before integration

- Use the [data registry](DATA_REGISTRY.md) for each node’s topic, payload fields, unit, reporting interval, and status messages. Its entries stay draft until owners, locations, and topics are confirmed. Keep hardware code, backend validation, and dashboard labels aligned when an approved entry changes.
- Use the instructor’s class broker and the assigned Block A topic tree. Keep credentials in ignored local files. Share sanitized payload examples and use placeholders in chat, screenshots, and committed sample files.
- Label test data clearly. Preserve the timestamp of the last valid reading and show stale, offline, or unknown states as appropriate. Restrict parking data and technician actions according to the [security checklist](SECURITY.md).
- Before wiring, confirm the exact ESP32 model, module pin labels, and signal voltage. KAHIMTANG’s HC-SR04 ECHO lines each need their own voltage divider before they reach an ESP32 input.

## Coding practices

| Good practice | Common mistake |
| --- | --- |
| Give pins and timing values meaningful names. Explain why a threshold or timeout was chosen. | Burying pin numbers and unexplained delays throughout a sketch. |
| Debounce buttons and use bounded timing so inputs keep working during reconnects. | Letting one press create several requests or trapping the board in a reconnect loop. |
| Validate incoming messages, use an allowlist for actuator commands, and put outputs in a safe state on startup and connection loss. | Trusting any incoming message to turn on an output or replaying alerts after reconnect. |
| Check each ultrasonic sensor in turn and handle a missing echo as an invalid reading. | Treating a timeout as an empty bay or letting the two sensors interfere with each other. |
| Keep changes focused on the assigned station. Check existing code before adding another library or changing a shared interface. | Replacing working teammate code or changing the topic, units, and dashboard separately. |
| Record what you actually ran, the observed result, and what still needs hardware. | Calling a firmware compile a successful live campus test. |

## What to include in a pull request

- The station, board model, wiring or pin map, and any library versions needed to reproduce the work.
- One sanitized example message with topic, payload, and expected behavior. For an actuator, add a valid command and a rejected-command case when tested.
- The result of the checks you ran in [TESTING.md](TESTING.md). Keep Serial Monitor, MQTT Explorer, and dashboard results distinct so the next teammate knows where to start.
- A short description of any shared change the lead needs to review before merging.

## Prompt you can paste into an AI chat

> I’m working on SENTRA, the MSU-IIT BCA188 Block A project. Read the attached AGENTS.md, DATA_REGISTRY.md, and the README for my station. My assignment is [station or node ID] and my ESP32 model is [model]. Help me with [one task] in my assigned folder. Follow our registered topics and units. Explain what I need to verify on my board and clearly label any part we haven’t tested.
