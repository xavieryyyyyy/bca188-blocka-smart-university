#pragma once
#include <cstdint>
#include <cstring>

enum class RequestPhase : uint8_t {
    Idle, WaitingForSave, Saved, WaitingForResolution
};

class RequestState {
public:
    static RequestState restore(uint64_t stored) {
        RequestState request;
        const uint64_t id = stored >> 8;
        const uint8_t phase = stored & 0xff;
        if (id && id <= UINT32_MAX &&
            phase >= static_cast<uint8_t>(RequestPhase::WaitingForSave) &&
            phase <= static_cast<uint8_t>(RequestPhase::WaitingForResolution)) {
            request.id_ = static_cast<uint32_t>(id);
            request.phase_ = static_cast<RequestPhase>(phase);
        }
        return request;
    }

    bool start(uint32_t id) {
        if (phase_ != RequestPhase::Idle || !id) return false;
        id_ = id;
        phase_ = RequestPhase::WaitingForSave;
        return true;
    }

    bool requestResolution() {
        if (phase_ != RequestPhase::Saved) return false;
        phase_ = RequestPhase::WaitingForResolution;
        return true;
    }

    bool confirm(uint32_t id, const char* action) {
        if (!id || id != id_) return false;
        if (phase_ == RequestPhase::WaitingForSave && !std::strcmp(action, "received")) {
            phase_ = RequestPhase::Saved;
            return true;
        }
        if ((phase_ == RequestPhase::WaitingForResolution ||
             phase_ == RequestPhase::WaitingForSave) && !std::strcmp(action, "resolved")) {
            *this = RequestState{};
            return true;
        }
        return false;
    }

    RequestPhase phase() const { return phase_; }
    uint32_t id() const { return id_; }
    uint64_t storedValue(uint32_t count) const { return (uint64_t(count) << 8) | static_cast<uint8_t>(phase_); }

private:
    uint32_t id_ = 0;
    RequestPhase phase_ = RequestPhase::Idle;
};
