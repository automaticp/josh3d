#pragma once
#include "EnumUtils.hpp" // IWYU pragma: keep


namespace josh::detail {


#define JOSH3D_DECLARE_ENUMS_EQ_COMPARABLE(Enum1, Enum2)                     \
    constexpr bool operator==(const Enum1& lhs, const Enum2& rhs) noexcept { \
        return to_underlying(lhs) == to_underlying(rhs);                     \
    }


// Will be extended with support for casting.
#define JOSH3D_DECLARE_ENUM_AS_SUPERSET(SupersetEnum, SubsetEnum) \
    JOSH3D_DECLARE_ENUMS_EQ_COMPARABLE(SupersetEnum, SubsetEnum)


#define JOSH3D_DECLARE_ENUMS_AS_EQUAL(Enum1, Enum2) \
    JOSH3D_DECLARE_ENUMS_EQ_COMPARABLE(Enum1, Enum2)

} // namespace josh::detail
