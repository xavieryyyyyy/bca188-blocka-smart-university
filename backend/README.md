# Campus backend

The backend team builds the Python service that receives registered messages from the instructor’s MQTT broker, stores readings and tickets, and sends live updates to the dashboard. ANDAM uses the service to save a ticket before the technician station plays its alert.

Start by reviewing the [data registry](../DATA_REGISTRY.md). Agree with the dashboard team on the data shape, access rules, and how offline and stale readings are represented. Keep broker credentials in local settings.
