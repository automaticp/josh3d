#pragma once
#include <cstddef>
#include <array>
#include "Events.hpp"
#include "FXType.hpp"

// A simple manager that both updates the state of each FX,
// and also sends out events whenever a certain effect
// becomes active/inactive.

class FXStateManager {
private:
    struct FX {
        float time{ -1.f };

        void enable(float duration) noexcept { time = duration; }
        void disable() noexcept { time = -1.f; }
        bool is_active() const noexcept { return time > 0.f; }

        // True if the update disabled the FX.
        bool update(float dt) noexcept {
            if (time > 0.f) {
                time -= dt;
                return time <= 0.f;
            }
            return false;
        }
    };

    std::array<FX, 7> effects_{};

public:
    void update(float dt) {
        for (size_t i{ 0 }; i < effects_.size(); ++i) {

            if (effects_[i].update(dt)) {
                events.push_fx_toggle_event(
                    { FXType{ i }, FXToggleType::disable }
                );
            }

        }
    }

    void enable_for(FXType type, float duration) {
        const bool new_event = !at(type).is_active();
        if (new_event) {
            events.push_fx_toggle_event({ type, FXToggleType::enable });
        }
        at(type).enable(duration);
    }

    void enable(FXType type) {
        // FIXME: Configure default durations.
        enable_for(type, 15.f);
    }

    bool is_active(FXType type) const noexcept {
        return at(type).is_active();
    }

private:
    FX& at(FXType type) noexcept {
        return effects_[static_cast<size_t>(type)];
    }

    const FX& at(FXType type) const noexcept {
        return effects_[static_cast<size_t>(type)];
    }
};
