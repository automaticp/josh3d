#pragma once
#include <cstdint>


namespace josh {


template<typename T, uint8_t MaxNumLODs>
struct LODPack {
    static constexpr uint8_t max_num_lods = MaxNumLODs;
    auto operator[](size_t i)       noexcept ->       T& { return lods[i]; }
    auto operator[](size_t i) const noexcept -> const T& { return lods[i]; }
    T       lods[max_num_lods];
    uint8_t max_lod;
    uint8_t min_lod;
    uint8_t cur_lod; // To save you padding. No other reason. Might be unused.
};


} // namespace josh
