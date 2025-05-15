#pragma once
#include <cstdint>
#include <cstddef>


namespace josh {


template<typename T, uint8_t MaxNumLODs>
struct LODPack {
    static constexpr uint8_t max_num_lods = MaxNumLODs;
    auto operator[](size_t i)       noexcept ->       T& { return lods[i]; }
    auto operator[](size_t i) const noexcept -> const T& { return lods[i]; }
    auto max()       noexcept ->       T& { return lods[max_lod]; }
    auto max() const noexcept -> const T& { return lods[max_lod]; }
    auto min()       noexcept ->       T& { return lods[min_lod]; }
    auto min() const noexcept -> const T& { return lods[min_lod]; }
    auto cur()       noexcept ->       T& { return lods[cur_lod]; }
    auto cur() const noexcept -> const T& { return lods[cur_lod]; }
    T       lods[max_num_lods];
    uint8_t max_lod;
    uint8_t min_lod;
    uint8_t cur_lod; // To save you padding. No other reason. Might be unused.
};


} // namespace josh
