#pragma once
#include "Math.hpp"
#include "GLAttributeTraits.hpp"
#include <tuple>
#include <cstddef>


namespace josh {


struct VertexStatic {
    using u16vec2 = glm::u16vec2;
    using i8vec3  = glm::i8vec3;

    vec3    position;
    u16vec2 uv;       // Packed half-floats.
    i8vec3  normal;   // Packed normalized ints representing float [-1, 1].
    i8vec3  tangent;  // Packed normalized ints representing float [-1, 1]

    // Create vertex from unpacked components.
    static auto pack(
        const vec3& position,
        const vec2& uv,
        const vec3& normal,
        const vec3& tangent)
            -> VertexStatic
    {
        using glm::int8, glm::uint8;
        return {
            .position = position,
            .uv       = glm::packHalf(uv),
            .normal   = glm::packSnorm<int8>(normal),
            .tangent  = glm::packSnorm<int8>(tangent),
        };
    }

    auto unpack_uv()      const noexcept -> vec2 { return glm::unpackHalf(uv);              }
    auto unpack_normal()  const noexcept -> vec3 { return glm::unpackSnorm<float>(normal);  }
    auto unpack_tangent() const noexcept -> vec3 { return glm::unpackSnorm<float>(tangent); }
};


template<> struct attribute_traits<VertexStatic> {
    // NOTE: The specs are reordered so that current shaders consume this correctly.
    // TODO: This should be reordered in shaders.
    using specs_type = std::tuple<
        AttributeSpecF,    // position
        AttributeSpecNorm, // normal
        AttributeSpecF,    // uv
        AttributeSpecNorm  // tangent
    >;

    static constexpr specs_type specs{
        AttributeSpecF   { AttributeTypeF::Float,     AttributeComponents::RGB, OffsetBytes{ offsetof(VertexStatic, position) } },
        AttributeSpecNorm{ AttributeTypeNorm::Byte,   AttributeComponents::RGB, OffsetBytes{ offsetof(VertexStatic, normal)   } },
        AttributeSpecF   { AttributeTypeF::HalfFloat, AttributeComponents::RG,  OffsetBytes{ offsetof(VertexStatic, uv)       } },
        AttributeSpecNorm{ AttributeTypeNorm::Byte,   AttributeComponents::RGB, OffsetBytes{ offsetof(VertexStatic, tangent)  } },
    };
};


} // namespace josh
