#pragma once
#include "GLAttributeTraits.hpp"
#include "VertexPNTTB.hpp"
#include <tuple>
#include <cstddef>


namespace josh {


template<> struct attribute_traits<VertexPNTTB> {
    using specs_type = std::tuple<
        AttributeSpecF,
        AttributeSpecF,
        AttributeSpecF,
        AttributeSpecF,
        AttributeSpecF
    >;

    static constexpr specs_type specs{
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RGB, OffsetBytes{ offsetof(VertexPNTTB, position)  } },
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RGB, OffsetBytes{ offsetof(VertexPNTTB, normal)    } },
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RG,  OffsetBytes{ offsetof(VertexPNTTB, tex_uv)    } },
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RGB, OffsetBytes{ offsetof(VertexPNTTB, tangent)   } },
        AttributeSpecF{ AttributeTypeF::Float, AttributeComponents::RGB, OffsetBytes{ offsetof(VertexPNTTB, bitangent) } },
    };
};




} // namespace josh
