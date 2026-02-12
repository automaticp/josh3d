#pragma once
#include "EnumUtils.hpp"
#include "GLAttributeTraits.hpp"
#include "KitchenSink.hpp"
#include "Scalars.hpp"
#include "Math.hpp"


namespace josh {

struct VertexStatic;
struct VertexSkinned;

/*
For now we use a simple fixed set of vertex formats.

This could be "upgraded" to an open compile-time defined set of layouts
with the help of the HashedString, similar to ResourceType, or even to
a fully runtime defined layout, although making shaders work with that
would be a major PITA given that they are currently 99% hand-written.
*/
enum class VertexFormat : u8
{
    Static,
    Skinned,
};
JOSH3D_DEFINE_ENUM_EXTRAS(VertexFormat, Static, Skinned);


template<VertexFormat V> struct vertex_type;
template<> struct vertex_type<VertexFormat::Static>  { using type = VertexStatic;  };
template<> struct vertex_type<VertexFormat::Skinned> { using type = VertexSkinned; };
template<VertexFormat V> using vertex_type_t = vertex_type<V>::type;

template<typename VertexT> struct vertex_format;
template<> struct vertex_format<VertexStatic>  : value_constant<VertexFormat::Static>  {};
template<> struct vertex_format<VertexSkinned> : value_constant<VertexFormat::Skinned> {};
template<typename VertexT> constexpr auto vertex_format_v = vertex_format<VertexT>::value;


struct VertexStatic
{
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

// FIXME: Attribute generation should be an external helper functionality on top of VAO. It's not really a GL thing.
// FIXME: Why specs_type typedef is even required? Just parse any tuple-like specs constant.
template<> struct attribute_traits<VertexStatic>
{
    // NOTE: The specs are reordered so that current shaders consume this correctly.
    // TODO: This should be reordered in shaders.
    using specs_type = std::tuple<
        AttributeSpecF,    // position
        AttributeSpecNorm, // normal
        AttributeSpecF,    // uv
        AttributeSpecNorm  // tangent
    >;

    static constexpr specs_type specs = {
        AttributeSpecF   { AttributeTypeF::Float,     AttributeComponents::RGB, OffsetBytes{ offsetof(VertexStatic, position) } },
        AttributeSpecNorm{ AttributeTypeNorm::Byte,   AttributeComponents::RGB, OffsetBytes{ offsetof(VertexStatic, normal)   } },
        AttributeSpecF   { AttributeTypeF::HalfFloat, AttributeComponents::RG,  OffsetBytes{ offsetof(VertexStatic, uv)       } },
        AttributeSpecNorm{ AttributeTypeNorm::Byte,   AttributeComponents::RGB, OffsetBytes{ offsetof(VertexStatic, tangent)  } },
    };
};


struct VertexSkinned
{
    using u8vec4  = glm::u8vec4;
    using u16vec2 = glm::u16vec2;
    using uvec4   = glm::uvec4;
    using i8vec3  = glm::i8vec3;

    vec3    position;
    u16vec2 uv;            // Packed half-floats.
    i8vec3  normal;        // Packed normalized ints representing floats from -1.0 to 1.0.
    i8vec3  tangent;       // Packed normalized ints representing floats from -1.0 to 1.0.
    u8vec4  joint_ids;     // Up-to 255 joints. TODO: Could be up-to 4095 joints if packed in 12 bits per joint.
    u8vec4  joint_weights; // Packed normalized uints representing floats from 0.0 to 1.0.

    // Create VertexSkinned from unpacked components.
    static auto pack(
        const vec3&  position,
        const vec2&  uv,
        const vec3&  normal,
        const vec3&  tangent,
        const uvec4& joint_ids,
        const vec4&  joint_weights)
            -> VertexSkinned
    {
        using glm::int8, glm::uint8;
        assert(glm::all(glm::lessThan(joint_ids, uvec4(255))));
        return {
            .position      = position,
            .uv            = glm::packHalf(uv),
            .normal        = glm::packSnorm<int8>(normal),
            .tangent       = glm::packSnorm<int8>(tangent),
            .joint_ids     = u8vec4(joint_ids),
            .joint_weights = glm::packUnorm<uint8>(joint_weights),
        };
    }

    auto unpack_uv()            const noexcept -> vec2 { return glm::unpackHalf(uv);                    }
    auto unpack_normal()        const noexcept -> vec3 { return glm::unpackSnorm<float>(normal);        }
    auto unpack_tangent()       const noexcept -> vec3 { return glm::unpackSnorm<float>(tangent);       }
    auto unpack_joint_weights() const noexcept -> vec4 { return glm::unpackUnorm<float>(joint_weights); }
};


template<> struct attribute_traits<VertexSkinned> {
    using specs_type = std::tuple<
        AttributeSpecF,    // position
        AttributeSpecF,    // uv
        AttributeSpecNorm, // normal
        AttributeSpecNorm, // tangent
        AttributeSpecI,    // joint_ids
        AttributeSpecNorm  // joint_weights
    >;

    static constexpr specs_type specs = {
        AttributeSpecF   { AttributeTypeF::Float,     AttributeComponents::RGB,  OffsetBytes{ offsetof(VertexSkinned, position)      } },
        AttributeSpecF   { AttributeTypeF::HalfFloat, AttributeComponents::RG,   OffsetBytes{ offsetof(VertexSkinned, uv)            } },
        AttributeSpecNorm{ AttributeTypeNorm::Byte,   AttributeComponents::RGB,  OffsetBytes{ offsetof(VertexSkinned, normal)        } },
        AttributeSpecNorm{ AttributeTypeNorm::Byte,   AttributeComponents::RGB,  OffsetBytes{ offsetof(VertexSkinned, tangent)       } },
        AttributeSpecI   { AttributeTypeI::UByte,     AttributeComponents::RGBA, OffsetBytes{ offsetof(VertexSkinned, joint_ids)     } },
        AttributeSpecNorm{ AttributeTypeNorm::UByte,  AttributeComponents::RGBA, OffsetBytes{ offsetof(VertexSkinned, joint_weights) } },
    };
};





} // namespace josh
