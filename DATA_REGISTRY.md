# SENTRA Data Registry

**Draft for team and instructor review.** Confirm the six node IDs, actual room and bay labels, and owner or co-owner for every node before publishing to the class broker. Check for duplicate measurements within Block A and with the other block.

## What each subsystem team fills in

1. Assign an owner or co-owner to each ESP32 in your group. Enter their names in the **Owner** column of every row for that board.
2. Replace **Location** placeholders with the actual campus labels agreed for the field test. HAYAG records its monitored classroom, ANDAM records both classrooms and the technician desk, and KAHIMTANG records each bay separately, even though both bays share one ESP32.
3. Open a pull request with your group’s rows. Confirm that the location, measurement, and proposed topic describe the same station. Get the registry entries confirmed before publishing to the class broker.

HAYAG, ANDAM, and KAHIMTANG can fill in their rows and start bench-testing in parallel. Share confirmed topics and sample messages with the project lead as each station is ready for dashboard integration.

| Node | Owner | Input or source | Measurement | Location | Purpose | Proposed MQTT topic | Unit | Interval |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| [n01](nodes/n01-hayag-climate/) | Pending | SHT30-D | Air temperature | HAYAG classroom, actual room ID pending | Show classroom temperature. | `bca188/blocka/hayag/n01/temperature` | C | On change and at least every 60 s while valid |
| [n01](nodes/n01-hayag-climate/) | Pending | SHT30-D | Relative humidity | HAYAG classroom, actual room ID pending | Show classroom humidity. | `bca188/blocka/hayag/n01/humidity` | % | On change and at least every 60 s while valid |
| [n01](nodes/n01-hayag-climate/) | Pending | HC-SR501 | Recent motion | HAYAG classroom, actual room ID pending | Show whether movement was detected recently. | `bca188/blocka/hayag/n01/recent-motion` | bool | On change and at least every 60 s while valid |
| [n02](nodes/n02-hayag-lighting/) | Pending | BH1750 | Illuminance at the window-side reference point | HAYAG classroom, actual room ID pending | Track ambient light at the window-side measurement point. | `bca188/blocka/hayag/n02/window-lux` | lx | On change and at least every 60 s while valid |
| [n02](nodes/n02-hayag-lighting/) | Pending | BH1750 | Illuminance at the desk surface | HAYAG classroom, actual room ID pending | Track light where people work. | `bca188/blocka/hayag/n02/desk-lux` | lx | On change and at least every 60 s while valid |
| [n03](nodes/n03-andam-classroom-1/) | Pending | Help button | Classroom 1 help requests | First ANDAM classroom, actual room ID pending | Create a ticket when the room’s count increases. | `bca188/blocka/andam/n03/help-request-count` | count | On change and at least every 60 s while valid |
| [n04](nodes/n04-andam-classroom-2/) | Pending | Help button | Classroom 2 help requests | Second ANDAM classroom, actual room ID pending | Create a ticket when the room’s count increases. | `bca188/blocka/andam/n04/help-request-count` | count | On change and at least every 60 s while valid |
| [n05](nodes/n05-andam-technician-desk/) | Pending | Buzzer receiver | Audible alerts played | Technician desk, actual location pending | Check that the desk receiver responded to new saved tickets. | `bca188/blocka/andam/n05/alert-count` | count | On change and at least every 60 s while valid |
| [n06](nodes/n06-kahimtang-parking/) | Pending | HC-SR04 | Bay 1 occupancy state | Bay 1, actual campus label pending | Show whether Bay 1 is available. | `bca188/blocka/kahimtang/n06/bay-1-occupied` | bool | On change and at least every 60 s while valid |
| [n06](nodes/n06-kahimtang-parking/) | Pending | HC-SR04 | Bay 2 occupancy state | Bay 2, actual campus label pending | Show whether Bay 2 is available. | `bca188/blocka/kahimtang/n06/bay-2-occupied` | bool | On change and at least every 60 s while valid |

## MQTT message rules

- Each ESP32 has its own client ID. Telemetry uses `bca188/blocka/<subsystem>/<node>/<metric>`.
- Each status topic uses `bca188/blocka/<subsystem>/<node>/status`. Send retained `online` on connection and configure retained `offline` as the last will.
- Send a valid reading when it changes enough to matter. If it stays steady, send a heartbeat at least once per minute. Target 45 seconds in firmware to leave time for scheduling.
- Use `0` for false and `1` for true on `bool` topics. A missing or stale parking reading should display **Unknown**.
- Record valid sensor ranges and thresholds after testing the actual mounting spots. Reject invalid values and keep the last valid reading timestamp.
- Repeated reports of an unchanged help-request count must keep the same ticket count. The ANDAM team needs to test counter persistence across restart and reconnection.

Example temperature message for `bca188/blocka/hayag/n01/temperature`.

```json
{"node":"n01","metric":"temperature","value":27.4,"unit":"C","uptime":52}
```

## Proposed actuator commands

| Topic | Allowed action |
| --- | --- |
| `bca188/blocka/andam/n03/cmd` | `received` to light the Classroom 1 feedback LED briefly after its ticket is saved |
| `bca188/blocka/andam/n04/cmd` | `received` to light the Classroom 2 feedback LED briefly after its ticket is saved |
| `bca188/blocka/andam/n05/cmd` | `beep` to play one short alert for a newly saved ticket |

The final command contract needs an agreed way to identify each ticket and suppress duplicate alerts. Keep commands non-retained, restrict command publishing to the approved service, and default outputs to off on boot and connection loss.
