#include <cassert>
#include <cstring>
#include <cstdint>
#include "../../nodes/n03-andam-classroom-1/include/RequestState.h"
#include "../../nodes/n05-andam-technician-desk/include/DeskAlerts.h"

static bool remember(uint32_t) { return true; }
static bool flashWriteFails(uint32_t) { return false; }

int main() {
    RequestState room;
    assert(room.start(7));
    assert(!room.start(8));
    assert(room.id() == 7 && room.phase() == RequestPhase::WaitingForSave);
    assert(!room.confirm(8, "received"));
    assert(room.confirm(7, "received"));
    assert(room.requestResolution());
    assert(room.id() == 7 && room.phase() == RequestPhase::WaitingForResolution);
    assert(!room.confirm(7, "received"));
    RequestState rebooted = RequestState::restore(room.storedValue(7));
    assert(rebooted.id() == 7 && rebooted.phase() == RequestPhase::WaitingForResolution);
    assert(rebooted.confirm(7, "resolved"));
    assert(rebooted.id() == 0 && rebooted.phase() == RequestPhase::Idle);

    assert(rebooted.storedValue(7) >> 8 == 7);
    assert(RequestState::restore(rebooted.storedValue(7)).phase() == RequestPhase::Idle);

    DeskAlerts desk(40);
    assert(!desk.update(10, true)); // Quiet at startup.
    assert(!desk.accept(40, remember)); // Duplicate across device restart.
    assert(!desk.accept(41, flashWriteFails));
    assert(desk.accept(41, remember));
    assert(!desk.accept(41, remember));
    assert(desk.update(20, true));
    assert(desk.update(139, true));
    assert(!desk.update(140, true));
    assert(!desk.update(1000, true)); // No reminder.
    assert(desk.accept(42, remember));
    assert(desk.accept(43, remember));
    assert(desk.update(1020, true));
    assert(!desk.update(1050, false)); // Cancel the active and queued alerts.
    assert(!desk.update(1500, true));
    assert(!desk.accept(43, remember)); // Replaying an alert after reconnect stays quiet.
    assert(desk.accept(44, remember));
    assert(desk.accept(45, remember));
    assert(desk.update(UINT32_MAX - 12, true));
    assert(!desk.update(107, true)); // Pulse stops across millis() wraparound.
    assert(desk.update(227, true)); // A separate accepted ticket gets its own pulse.
    assert(desk.played() == 4);
}
