#pragma once
#include "Scalars.hpp"


namespace josh {


template<typename T, u8 MaxNumLODs>
struct LODPack
{
    static constexpr u8 max_num_lods = MaxNumLODs;
    auto operator[](uindex i)       noexcept ->       T& { return lods[i]; }
    auto operator[](uindex i) const noexcept -> const T& { return lods[i]; }
    auto max()       noexcept ->       T& { return lods[max_lod]; }
    auto max() const noexcept -> const T& { return lods[max_lod]; }
    auto min()       noexcept ->       T& { return lods[min_lod]; }
    auto min() const noexcept -> const T& { return lods[min_lod]; }
    auto cur()       noexcept ->       T& { return lods[cur_lod]; }
    auto cur() const noexcept -> const T& { return lods[cur_lod]; }
    T  lods[max_num_lods];
    u8 max_lod = 0;
    u8 min_lod = 0;
    u8 cur_lod = 0; // To save you padding. No other reason. Might be unused.
};


} // namespace josh
