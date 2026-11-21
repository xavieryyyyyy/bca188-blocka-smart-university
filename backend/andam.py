"""Save ANDAM tickets and send signed replies through the registered MQTT topics."""

from contextlib import closing
import hmac
import json
import logging
import os
from pathlib import Path
import sqlite3
from threading import Event, Lock
import time


PREFIX = "bca188/blocka/andam"
ROOM_BITS = {"n03": 1, "n04": 2}
METRIC = "help-request-count"
MAX_AGE_SECONDS = 60
SNAPSHOT_INTERVAL_SECONDS = 3
DESK_COMMAND = f"{PREFIX}/n05/cmd"
logger = logging.getLogger(__name__)


class TicketStore:
    def __init__(self, path):
        self.lock = Lock()
        self.db = sqlite3.connect(path, check_same_thread=False)
        with self.db:
            self.db.execute("""
                CREATE TABLE IF NOT EXISTS andam_tickets (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    node TEXT NOT NULL,
                    request_id INTEGER NOT NULL,
                    opened_at INTEGER NOT NULL,
                    resolved_at INTEGER,
                    UNIQUE (node, request_id)
                )
            """)

    def close(self):
        self.db.close()

    def record(self, node, request, event, now):
        with self.lock, self.db:
            ticket = self.db.execute("""
                SELECT id, resolved_at FROM andam_tickets
                WHERE node = ? AND request_id = ?
            """, (node, request)).fetchone()
            if event == "resolve":
                if not ticket:
                    return None, None
                self.db.execute("""
                    UPDATE andam_tickets SET resolved_at = ?
                    WHERE id = ? AND resolved_at IS NULL
                """, (now, ticket[0]))
                return "resolved", None
            if ticket:
                return ("resolved" if ticket[1] is not None else "received"), None
            row = self.db.execute("""
                INSERT INTO andam_tickets (node, request_id, opened_at)
                VALUES (?, ?, ?)
            """, (node, request, now))
            return "received", row.lastrowid

    def open_rooms(self):
        with self.lock:
            return {
                row[0] for row in self.db.execute("""
                    SELECT DISTINCT node FROM andam_tickets WHERE resolved_at IS NULL
                """)
            }


def signature(key, *fields):
    return hmac.digest(key, "|".join(map(str, fields)).encode(), "sha256").hex()


class AndamService:
    def __init__(self, store, room_keys, desk_key):
        self.store = store
        self.room_keys = room_keys
        self.desk_key = desk_key
        self.topics = {f"{PREFIX}/{node}/{METRIC}": node for node in room_keys}

    @staticmethod
    def command(key, node, action, field, value, now):
        return {
            "node": node, "action": action, field: value, "ts": now,
            "sig": signature(key, node, action, value, now),
        }

    def room_snapshot(self, now):
        rooms = sum(ROOM_BITS[node] for node in self.store.open_rooms())
        return self.command(self.desk_key, "n05", "rooms", "rooms", rooms, now)

    def receive(self, topic, payload, *, retained, now):
        node = self.topics.get(topic)
        if node is None or retained or len(payload) > 512:
            return []
        try:
            data = json.loads(payload)
        except (ValueError, RecursionError):
            return []
        if not isinstance(data, dict) or (
            data.get("node"), data.get("metric"), data.get("unit")
        ) != (node, METRIC, "count"):
            return []

        event, request = data.get("event"), data.get("value")
        uptime, timestamp, received_signature = data.get("uptime"), data.get("ts"), data.get("sig")
        if event not in ("help_request", "resolve", "heartbeat"):
            return []
        if any(type(value) is not int or not 0 <= value <= 0xffffffff for value in
               (request, uptime, timestamp)):
            return []
        if not timestamp or abs(now - timestamp) > MAX_AGE_SECONDS:
            return []
        key = self.room_keys[node]
        expected = signature(key, node, event, request, uptime, timestamp)
        if not isinstance(received_signature, str) or not received_signature.isascii() or not (
            hmac.compare_digest(received_signature, expected)
        ):
            return []
        if event == "heartbeat" or request == 0:
            return []

        action, ticket_id = self.store.record(node, request, event, now)
        if action is None:
            return []
        messages = [(f"{PREFIX}/{node}/cmd", self.command(key, node, action, "req", request, now))]
        if ticket_id is not None:
            messages.append((DESK_COMMAND, self.command(
                self.desk_key, "n05", "beep", "ticket", ticket_id, now
            )))
        return messages


def load_key(variable):
    value = os.environ[variable]
    key = bytes.fromhex(value)
    if len(value) != 64 or len(key) != 32:
        raise ValueError(f"{variable} must contain 64 hexadecimal characters.")
    return key


def run(service):
    from paho.mqtt import client as mqtt

    subscriber = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="sentra-andam-service")
    subscriber.username_pw_set(os.environ["ANDAM_MQTT_USERNAME"], os.environ["ANDAM_MQTT_PASSWORD"])
    tls = os.environ.get("ANDAM_MQTT_TLS", "true").lower()
    if tls not in ("true", "false"):
        raise ValueError("ANDAM_MQTT_TLS must be true or false.")
    if tls == "true":
        subscriber.tls_set(ca_certs=os.environ.get("ANDAM_MQTT_CA_FILE") or None)

    def publish(topic, command):
        # Live desk commands use QoS 0 and are never retained. Room requests
        # retry until confirmed, and room LED snapshots refresh periodically.
        result = subscriber.publish(topic, json.dumps(command, separators=(",", ":")),
                                    qos=0, retain=False)
        if result.rc != mqtt.MQTT_ERR_SUCCESS:
            logger.warning("Could not send command to %s (%s)", topic, result.rc)

    def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            logger.warning("MQTT connection refused: %s", reason_code)
            return
        logger.info("Connected to MQTT")
        for topic in service.topics:
            client.subscribe(topic, qos=1)

    def on_subscribe(client, userdata, mid, reason_codes, properties):
        if any(code.is_failure for code in reason_codes):
            logger.error("Broker refused an ANDAM subscription: %s", reason_codes)

    def on_message(client, userdata, message):
        try:
            commands = service.receive(
                message.topic, message.payload, retained=message.retain, now=int(time.time())
            )
        except sqlite3.Error:
            logger.exception("Could not save ANDAM ticket. The classroom will retry.")
            return
        for topic, command in commands:
            publish(topic, command)

    subscriber.on_connect = on_connect
    subscriber.on_subscribe = on_subscribe
    subscriber.on_message = on_message
    subscriber.connect_async(
        os.environ["ANDAM_MQTT_HOST"], int(os.environ.get("ANDAM_MQTT_PORT", "8883"))
    )
    subscriber.loop_start()
    try:
        timer = Event()
        while not timer.wait(SNAPSHOT_INTERVAL_SECONDS):
            if subscriber.is_connected():
                publish(DESK_COMMAND, service.room_snapshot(int(time.time())))
    except KeyboardInterrupt:
        pass
    finally:
        subscriber.disconnect()
        subscriber.loop_stop()


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    keys = {"n03": load_key("ANDAM_N03_HMAC_KEY_HEX")}
    if os.environ.get("ANDAM_N04_HMAC_KEY_HEX"):
        keys["n04"] = load_key("ANDAM_N04_HMAC_KEY_HEX")
    desk_key = load_key("ANDAM_N05_HMAC_KEY_HEX")
    default_db = Path(__file__).with_name("data") / "andam.sqlite3"
    database = Path(os.environ.get("ANDAM_DB_PATH") or default_db)
    database.parent.mkdir(parents=True, exist_ok=True)
    with closing(TicketStore(database)) as tickets:
        run(AndamService(tickets, keys, desk_key))
