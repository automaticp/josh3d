#pragma once
#include "Math.hpp"
#include "GLAttributeTraits.hpp"
#include <tuple>
#include <cstddef>


namespace josh {


struct VertexPNUTB {
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec3 tangent;
    vec3 bitangent;
};


template<> struct attribute_traits<VertexPNUTB> {
    using specs_type = std::tuple<
        AttributeSpecF, // position
        AttributeSpecF, // normal
        AttributeSpecF, // uv
        AttributeSpecF, // tangent
        AttributeSpecF  // bitangent
    >;

    static constexpr specs_type specs{
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RGB, OffsetBytes{ offsetof(VertexPNUTB, position)  } },
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RGB, OffsetBytes{ offsetof(VertexPNUTB, normal)    } },
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RG,  OffsetBytes{ offsetof(VertexPNUTB, uv)        } },
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RGB, OffsetBytes{ offsetof(VertexPNUTB, tangent)   } },
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RGB, OffsetBytes{ offsetof(VertexPNUTB, bitangent) } },
    };
};


} // namespace josh
