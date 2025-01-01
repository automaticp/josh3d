#pragma once
#include "GLAPICommonTypes.hpp"
#include "Math.hpp"
#include "GLAttributeTraits.hpp"
#include <glm/fwd.hpp>
#include <tuple>
#include <cstddef>


namespace josh {


struct VertexSkinned {
    vec3 position;
    vec2 uv;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;        // FIXME: This is unnecessary, bleugh.
    glm::u8vec4 joint_ids; // Up-to 255 joints.
    vec3 joint_weights;    // w[3] is implicit as (1 - (w[0] + w[1] + w[2]))
    // TODO: Weights can just be normalized uint8[3].
};


template<> struct attribute_traits<VertexSkinned> {
    using specs_type = std::tuple<
        AttributeSpecF, // position
        AttributeSpecF, // uv
        AttributeSpecF, // normal
        AttributeSpecF, // tangent
        AttributeSpecF, // bitangent
        AttributeSpecI, // joint_ids
        AttributeSpecF  // joint_weights
    >;

    static constexpr specs_type specs{
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RGB,  OffsetBytes{ offsetof(VertexSkinned, position)      } },
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RG,   OffsetBytes{ offsetof(VertexSkinned, uv)            } },
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RGB,  OffsetBytes{ offsetof(VertexSkinned, normal)        } },
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RGB,  OffsetBytes{ offsetof(VertexSkinned, tangent)       } },
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RGB,  OffsetBytes{ offsetof(VertexSkinned, bitangent)     } },
        AttributeSpecI{ AttributeTypeI::UByte, AttributeComponents::RGBA, OffsetBytes{ offsetof(VertexSkinned, joint_ids)     } },
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RGB,  OffsetBytes{ offsetof(VertexSkinned, joint_weights) } },
    };
};


} // namespace josh
