#pragma once
#include <cstdint>

class DeskAlerts {
public:
    explicit DeskAlerts(uint32_t lastTicket = 0) : lastTicket_(lastTicket) {}

    bool accept(uint32_t ticket, bool (*remember)(uint32_t)) {
        if (ticket <= lastTicket_ || !remember(ticket)) return false;
        lastTicket_ = ticket;
        ++pending_;
        return true;
    }

    bool update(uint32_t now, bool connected) {
        if (!connected) {
            silence();
        } else if (active_ && now - changedAt_ >= PULSE_MS) {
            active_ = false;
            waiting_ = true;
            changedAt_ = now;
        } else if (!active_ && pending_ && (!waiting_ || now - changedAt_ >= PULSE_MS)) {
            --pending_;
            ++played_;
            active_ = true;
            waiting_ = false;
            changedAt_ = now;
        }
        return active_;
    }

    void silence() {
        pending_ = 0;
        active_ = false;
        waiting_ = false;
    }

    uint32_t played() const { return played_; }

private:
    static constexpr uint32_t PULSE_MS = 120;
    uint32_t lastTicket_;
    uint32_t pending_ = 0;
    uint32_t played_ = 0;
    uint32_t changedAt_ = 0;
    bool active_ = false;
    bool waiting_ = false;
};
