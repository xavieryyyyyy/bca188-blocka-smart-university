import hashlib
import hmac
import json
from pathlib import Path
import tempfile
import unittest

from andam import AndamService, TicketStore

NOW = 1_800_000_001
CLASSROOM_KEY = b"\x10" * 32
DESK_KEY = b"\x20" * 32
COUNT_TOPIC = "bca188/blocka/andam/n03/help-request-count"


class AndamTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.database = Path(directory.name) / "tickets.sqlite3"
        self.open_service()

    def open_service(self):
        store = TicketStore(self.database)
        self.addCleanup(store.close)
        self.service = AndamService(store, {"n03": CLASSROOM_KEY}, DESK_KEY)

    def send(self, event, request=9, timestamp=NOW, topic=COUNT_TOPIC, retained=False,
             key=CLASSROOM_KEY):
        signed = f"n03|{event}|{request}|15|{timestamp}".encode()
        payload = json.dumps({
            "node": "n03", "metric": "help-request-count", "unit": "count",
            "event": event, "value": request, "uptime": 15, "ts": timestamp,
            "sig": hmac.digest(key, signed, "sha256").hex(),
        }).encode()
        return dict(self.service.receive(topic, payload, retained=retained, now=NOW))

    def test_first_request_saves_ticket_and_sends_one_signed_beep(self):
        messages = self.send("help_request")
        saved = messages["bca188/blocka/andam/n03/cmd"]
        beep = messages["bca188/blocka/andam/n05/cmd"]
        self.assertEqual((saved["action"], saved["req"]), ("received", 9))
        self.assertEqual((beep["action"], beep["ticket"]), ("beep", 1))
        expected = hmac.new(DESK_KEY, f"n05|beep|1|{NOW}".encode(), hashlib.sha256)
        self.assertEqual(beep["sig"], expected.hexdigest())
        self.assertEqual(self.service.room_snapshot(NOW)["rooms"], 1)

    def test_retry_after_restart_replies_without_a_second_beep(self):
        self.send("help_request")
        self.open_service()
        messages = self.send("help_request")
        self.assertEqual(list(messages), ["bca188/blocka/andam/n03/cmd"])
        self.assertEqual(messages["bca188/blocka/andam/n03/cmd"]["action"], "received")

    def test_resolve_is_durable_and_idempotent(self):
        self.assertEqual(self.send("resolve"), {})
        self.send("help_request")
        self.assertEqual(self.send("resolve")["bca188/blocka/andam/n03/cmd"]["action"], "resolved")
        self.open_service()
        self.assertEqual(self.service.room_snapshot(NOW)["rooms"], 0)
        self.assertEqual(self.send("resolve")["bca188/blocka/andam/n03/cmd"]["action"], "resolved")
        self.assertEqual(self.send("help_request")["bca188/blocka/andam/n03/cmd"]["action"],
                         "resolved")

    def test_new_ticket_after_resolve_gets_a_distinct_alert_id(self):
        self.send("help_request")
        self.send("resolve")
        messages = self.send("help_request", request=10)
        self.assertEqual(messages["bca188/blocka/andam/n05/cmd"]["ticket"], 2)

    def test_heartbeat_and_invalid_input_do_not_create_tickets(self):
        for args in (
            ("heartbeat", {}),
            ("help_request", {"topic": "sentra/andam/n03/request"}),
            ("help_request", {"retained": True}),
            ("help_request", {"key": DESK_KEY}),
            ("help_request", {"timestamp": NOW - 61}),
        ):
            with self.subTest(event=args[0], options=args[1]):
                self.assertEqual(self.send(args[0], **args[1]), {})
        self.assertEqual(self.service.room_snapshot(NOW)["rooms"], 0)
