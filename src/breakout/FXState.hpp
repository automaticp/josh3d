#pragma once
#include <cstddef>
#include <array>
#include <type_traits>


enum class FXType : size_t {
    shake = 0,
    speed = 1,
    sticky = 2,
    pass_through = 3,
    pad_size_up = 4,
    confuse = 5,
    chaos = 6
};


class FXState {
private:
    struct FX {
        float time{ 0.f };
        bool enabled{ false };

        void enable(float duration) noexcept {
            enabled = true;
            time = duration;
        }

        void update(float dt) noexcept {
            if (time > 0.f) {
                time -= dt;
            } else [[likely]] {
                enabled = false;
            }
        }
    };

    std::array<FX, 7> effects_{};

    FX& get_fx(FXType type) noexcept {
        return effects_[static_cast<std::underlying_type_t<FXType>>(type)];
    }

public:
    void update(float dt) noexcept {
        for (auto& fx : effects_) {
            fx.update(dt);
        }
    }

    void enable(FXType type, float duration) {
        get_fx(type).enable(duration);
    }

    bool is_active(FXType type) {
        return get_fx(type).enabled;
    }

};

