# SENTRA testing

Start with one node in Serial Monitor. Check the same node in MQTT Explorer before checking the dashboard. Add the actual result and evidence after running each test.

| ID | Test | What to do | Expected result | Actual result |
| --- | --- | --- | --- | --- |
| T01 | Node startup | Power each approved board and check its MQTT status topic. | The board reports retained online status under its own ID. | Pending |
| T02 | HAYAG sensors | Compare temperature and humidity with a reference. Cover each light sensor separately and walk past the PIR. | The intended measurement changes. Each reading belongs to the correct node and location. | Pending |
| T03 | ANDAM request | Press each classroom button once and observe the desk receiver. | One ticket for the correct room and one short beep for that ticket. | Pending |
| T04 | Parking bays | Check both bays empty, then place a vehicle in one bay at a time. | Only the correct bay becomes occupied. | Pending |
| T05 | Heartbeat | Listen to a steady node for two minutes. | Valid readings continue to arrive at least once a minute. | Pending |
| T06 | Wi-Fi recovery | Briefly disconnect the node for 30 seconds, then restore its Wi-Fi. | It reconnects and resumes reporting automatically. Record how the dashboard showed the interruption. | Pending |
| T07 | Broker recovery | With the instructor, observe recovery after a class broker restart. | Nodes reconnect, resubscribe where needed, and resume reporting. | Pending |
| T08 | Offline state | Unplug a node and watch its status topic and the dashboard. | The broker publishes its offline last will and the dashboard shows it as offline. | Pending |
| T09 | Invalid input | Send an invalid or overlong command using the authorized test setup. | The command is rejected. Outputs stay in a safe state. | Pending |
| T10 | Sensor fault | Simulate a disconnected sensor with the power off during wiring changes. | The node reports the fault. Missing or expired readings appear as Unknown or stale. | Pending |
| T11 | Remote view | Trigger a change at school and observe the dashboard from mobile data after cloud setup. | The authorized view updates. Record the actual delay. | Pending |
| T12 | Access controls | Try viewing and changing protected information with each approved account type. | Parking and ticket actions follow their assigned permissions. | Pending |
