#pragma once
#include "GLAttributeTraits.hpp"
#include "VertexPNUTB.hpp"
#include <tuple>
#include <cstddef>


namespace josh {


template<> struct attribute_traits<VertexPNUTB> {
    using specs_type = std::tuple<
        AttributeSpecF,
        AttributeSpecF,
        AttributeSpecF,
        AttributeSpecF,
        AttributeSpecF
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
