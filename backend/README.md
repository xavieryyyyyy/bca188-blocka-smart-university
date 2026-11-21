# Campus backend

The backend team builds the Python service that receives registered messages from the instructor’s MQTT broker, stores readings and tickets, and sends live updates to the dashboard. Start with the [data registry](../DATA_REGISTRY.md) for the shared topics and access rules.

## ANDAM ticket service

The [ANDAM service](andam.py) runs as a separate MQTT client. It stores tickets in SQLite before sending an MQTT reply to the classroom. Each node and request count identify one ticket. Retrying a saved request sends its reply again. The desk gets one live alert command when the ticket is first saved and periodic room LED updates from open tickets.

### Connect to the test broker

1. Install the dependency with `python -m pip install -r backend/requirements-andam.txt`.
2. Set the following environment variables locally. Use a dedicated MQTT account for the service, and generate a different 32-byte hexadecimal signing key for each node. Each node's ignored `include/secrets.h` uses its own matching key.
3. Run from the repository root with `python backend/andam.py`. Run one service instance and keep its SQLite file between restarts.

| Setting | Value |
| --- | --- |
| ANDAM_MQTT_HOST | EMQX deployment hostname from its overview |
| ANDAM_MQTT_PORT | 8883 for EMQX Cloud MQTT over TLS |
| ANDAM_MQTT_USERNAME and ANDAM_MQTT_PASSWORD | Dedicated service MQTT credentials |
| ANDAM_N03_HMAC_KEY_HEX | ICT3D signing key, 64 hexadecimal characters |
| ANDAM_N05_HMAC_KEY_HEX | Technician desk signing key, 64 hexadecimal characters |
| ANDAM_DB_PATH | Optional path to a persistent SQLite file. Defaults to backend/data/andam.sqlite3. |
| ANDAM_MQTT_CA_FILE | Optional CA bundle path if a private broker uses a CA outside the system trust store |
| ANDAM_MQTT_TLS | Optional. Defaults to true. Set to false for a broker confined to your computer during tests. |
| ANDAM_N04_HMAC_KEY_HEX | Optional. Leave unset until the second classroom uses the same contract. |

Give each account only the following permissions under the broker's default-deny rules. Every listed path is relative to the prefix `bca188/blocka/andam/`.

| Account | Publish | Subscribe |
| --- | --- | --- |
| ICT3D (n03) | n03/help-request-count, n03/status | n03/cmd |
| Desk (n05) | n05/alert-count, n05/status | n05/cmd |
| ANDAM service | n03/cmd, n05/cmd | n03/help-request-count |
| Existing MQTTX monitor | None | n03/help-request-count |

Keep the database when moving the service to a new broker. It holds the ticket IDs the desk uses to recognize old alerts. If a test database is intentionally reset, the desk's stored last ticket ID must be reset as part of the same test reset.

A desk that is offline when a ticket is saved will show the open room when it reconnects. Live beeps are sent without retention on the desk's clean MQTT session, so the room LEDs still show missed requests without ringing on reconnect. The service runs independently of the dashboard.

### Run the regression tests

Run `python -m unittest discover -s tests -p test_andam.py` from the backend folder. To check the classroom and desk logic on a development computer with a C++ compiler, compile [the firmware test](tests/andam_firmware_test.cpp) with C++11 and run the resulting executable. Use [T03](../TESTING.md) for the physical button and buzzer test.
