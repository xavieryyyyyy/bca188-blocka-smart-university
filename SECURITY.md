# SENTRA security and privacy checks

Each row is pending until the team records evidence from its own implementation or field test.

| Check | What to verify | Status |
| --- | --- | --- |
| Credentials | Wi-Fi, broker, and cloud credentials are stored in ignored local files. Samples contain placeholders. | Pending |
| MQTT access | Each ESP32 has a unique client ID. Topic access stays within the instructor-approved Block A scope. | Pending |
| Sensor and command validation | The service checks topic, node, unit, type, and payload size. Actuator nodes reject invalid commands and return to a safe state. | Pending |
| Dashboard access | Authorized faculty and staff can see KAHIMTANG. ANDAM ticket updates require technician access. Check the rules over the API as well as in the interface. | Pending |
| Data collection | Record classroom conditions, recent movement, help requests, and bay state. Keep identifying information out of telemetry and public test evidence. | Pending |
| Off-campus access | Use the approved cloud data and access rules. Confirm that expired readings have a visible age or stale state. | Pending |
| Campus field test | Confirm the locations with the instructor, protect wiring, and keep deployment details and credentials out of screenshots. | Pending |

For evidence, record the check, who verified it, and the supporting test or screenshot. Link relevant results from [TESTING.md](TESTING.md).
