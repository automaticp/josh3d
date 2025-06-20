#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "EnumUtils.hpp"
#include "GLAPI.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLScalars.hpp"
#include "detail/EnumCompatability.hpp"
#include <type_traits>
#include <tuple>


namespace josh {


enum class AttributeTypeF : GLuint
{
    Fixed                = GLuint(gl::GL_FIXED),
    Float                = GLuint(gl::GL_FLOAT),
    HalfFloat            = GLuint(gl::GL_HALF_FLOAT),
    Double               = GLuint(gl::GL_DOUBLE),
    UInt_10F_11F_11F_Rev = GLuint(gl::GL_UNSIGNED_INT_10F_11F_11F_REV),
};
JOSH3D_DEFINE_ENUM_EXTRAS(AttributeTypeF,
    Fixed, Float, HalfFloat, Double, UInt_10F_11F_11F_Rev);

enum class AttributeTypeNorm : GLuint
{
    Byte                = GLuint(gl::GL_BYTE),
    UByte               = GLuint(gl::GL_UNSIGNED_BYTE),
    Short               = GLuint(gl::GL_SHORT),
    UShort              = GLuint(gl::GL_UNSIGNED_SHORT),
    Int                 = GLuint(gl::GL_INT),
    UInt                = GLuint(gl::GL_UNSIGNED_INT),
    Int_2_10_10_10_Rev  = GLuint(gl::GL_INT_2_10_10_10_REV),
    UInt_2_10_10_10_Rev = GLuint(gl::GL_UNSIGNED_INT_2_10_10_10_REV),
};
JOSH3D_DEFINE_ENUM_EXTRAS(AttributeTypeNorm,
    Byte, UByte, Short, UShort, Int, UInt, Int_2_10_10_10_Rev, UInt_2_10_10_10_Rev);


enum class AttributeTypeBGRA : GLuint
{
    UByte               = GLuint(gl::GL_UNSIGNED_BYTE),
    Int_2_10_10_10_Rev  = GLuint(gl::GL_INT_2_10_10_10_REV),
    UInt_2_10_10_10_Rev = GLuint(gl::GL_UNSIGNED_INT_2_10_10_10_REV),
};
JOSH3D_DEFINE_ENUM_EXTRAS(AttributeTypeBGRA,
    UByte, Int_2_10_10_10_Rev, UInt_2_10_10_10_Rev);

enum class AttributeTypeI : GLuint
{
    Byte   = GLuint(gl::GL_BYTE),
    UByte  = GLuint(gl::GL_UNSIGNED_BYTE),
    Short  = GLuint(gl::GL_SHORT),
    UShort = GLuint(gl::GL_UNSIGNED_SHORT),
    Int    = GLuint(gl::GL_INT),
    UInt   = GLuint(gl::GL_UNSIGNED_INT),
};
JOSH3D_DEFINE_ENUM_EXTRAS(AttributeTypeI,
    Byte, UByte, Short, UShort, Int, UInt);

enum class AttributeTypeD : GLuint
{
    Double = GLuint(gl::GL_DOUBLE),
};
JOSH3D_DEFINE_ENUM_EXTRAS(AttributeTypeD, Double);

/*
Used as a return type, not as function argument.
Union set of all `AttributeType*`.
*/
enum class AttributeType : GLuint
{
    Fixed                = GLuint(gl::GL_FIXED),
    Float                = GLuint(gl::GL_FLOAT),
    HalfFloat            = GLuint(gl::GL_HALF_FLOAT),
    Double               = GLuint(gl::GL_DOUBLE),
    UInt_10F_11F_11F_Rev = GLuint(gl::GL_UNSIGNED_INT_10F_11F_11F_REV),
    Byte                 = GLuint(gl::GL_BYTE),
    UByte                = GLuint(gl::GL_UNSIGNED_BYTE),
    Short                = GLuint(gl::GL_SHORT),
    UShort               = GLuint(gl::GL_UNSIGNED_SHORT),
    Int                  = GLuint(gl::GL_INT),
    UInt                 = GLuint(gl::GL_UNSIGNED_INT),
    Int_2_10_10_10_Rev   = GLuint(gl::GL_INT_2_10_10_10_REV),
    UInt_2_10_10_10_Rev  = GLuint(gl::GL_UNSIGNED_INT_2_10_10_10_REV),
};
JOSH3D_DEFINE_ENUM_EXTRAS(AttributeType,
    Fixed, Float, HalfFloat, Double, UInt_10F_11F_11F_Rev, Byte, UByte,
    Short, UShort, Int, UInt, Int_2_10_10_10_Rev, UInt_2_10_10_10_Rev);

JOSH3D_DECLARE_ENUM_AS_SUPERSET(AttributeType, AttributeTypeF)
JOSH3D_DECLARE_ENUM_AS_SUPERSET(AttributeType, AttributeTypeNorm)
JOSH3D_DECLARE_ENUM_AS_SUPERSET(AttributeType, AttributeTypeBGRA)
JOSH3D_DECLARE_ENUM_AS_SUPERSET(AttributeType, AttributeTypeI)
JOSH3D_DECLARE_ENUM_AS_SUPERSET(AttributeType, AttributeTypeD)


enum class AttributeComponents : GLint
{
    Red  = 1,
    RG   = 2,
    RGB  = 3,
    RGBA = 4,
};
JOSH3D_DEFINE_ENUM_EXTRAS(AttributeComponents, Red, RG, RGB, RGBA);

enum class AttributeComponentsBGRA : GLint
{
    BGRA = GLint(gl::GL_BGRA),
};
JOSH3D_DEFINE_ENUM_EXTRAS(AttributeComponentsBGRA, BGRA);

enum class AttributeComponentsAll : GLint
{
    Red  = 1,
    RG   = 2,
    RGB  = 3,
    RGBA = 4,
    BGRA = GLint(gl::GL_BGRA),
};
JOSH3D_DEFINE_ENUM_EXTRAS(AttributeComponentsAll, Red, RG, RGB, RGBA, BGRA);

JOSH3D_DECLARE_ENUM_AS_SUPERSET(AttributeComponentsAll, AttributeComponents)
JOSH3D_DECLARE_ENUM_AS_SUPERSET(AttributeComponentsAll, AttributeComponentsBGRA)


struct AttributeSpecF
{
    AttributeTypeF      type;
    AttributeComponents components;
    OffsetBytes         offset_bytes;
};

struct AttributeSpecNorm
{
    AttributeTypeNorm   type;
    AttributeComponents components;
    OffsetBytes         offset_bytes;
};

struct AttributeSpecBGRA
{
    AttributeTypeBGRA       type;
    AttributeComponentsBGRA components;
    OffsetBytes             offset_bytes;
};

struct AttributeSpecFCast
{
    AttributeTypeNorm   type;
    AttributeComponents components;
    OffsetBytes         offset_bytes;
};

struct AttributeSpecI
{
    AttributeTypeI      type;
    AttributeComponents components;
    OffsetBytes         offset_bytes;
};

struct AttributeSpecD
{
    AttributeTypeD      type;
    AttributeComponents components;
    OffsetBytes         offset_bytes;
};


/*
Customization point for custom vertex attribute layouts.

Example specializations:

    struct MyVertex
    {
        float    xyz[3];
        uint32_t uv[2]; // Normalized UInt
        double   normals[3];
    };

    template<> struct attribute_traits<MyVertex>
    {
        using specs_type = std::tuple<
            AttributeSpecF,
            AttributeSpecNorm,
            AttributeSpecD
        >;

        static constexpr specs_type specs{
            AttributeSpecF   { AttributeTypeF::Float,   AttributeComponents::RGB, OffsetBytes{ offsetof(MyVertex, xyz)     } },
            AttributeSpecNorm{ AttributeTypeNorm::UInt, AttributeComponents::RG,  OffsetBytes{ offsetof(MyVertex, uv)      } },
            AttributeSpecD   { AttributeTypeD::Double,  AttributeComponents::RGB, OffsetBytes{ offsetof(MyVertex, normals) } }
        };
    };

    struct PackedVertex
    {
        float    xyz[3];
        uint16_t uv[2];
        int8_t   normals_xy[2];
        int8_t   tangents_xy[2];
    };

    template<> struct attribute_traits<PackedVertex>
    {
        using specs_type = std::tuple<
            AttributeSpecF,
            AttributeSpecNorm,
            AttributeSpecNorm,
            AttributeSpecNorm
        >;

        static constexpr specs_type specs{
            AttributeSpecF   { AttributeTypeF::Float,     AttributeComponents::RGB, OffsetBytes{ offsetof(PackedVertex, xyz)         } },
            AttributeSpecNorm{ AttributeTypeNorm::UShort, AttributeComponents::RG,  OffsetBytes{ offsetof(PackedVertex, uv)          } },
            AttributeSpecNorm{ AttributeTypeNorm::Byte,   AttributeComponents::RG,  OffsetBytes{ offsetof(PackedVertex, normals_xy)  } },
            AttributeSpecNorm{ AttributeTypeNorm::Byte,   AttributeComponents::RG,  OffsetBytes{ offsetof(PackedVertex, tangents_xy) } },
        };
    };

*/
template<typename VertexT>
struct attribute_traits;


namespace detail {

template<typename ...AttribSpecTs>
concept valid_attribute_specs =
    (any_of<AttribSpecTs,
        AttributeSpecF, AttributeSpecNorm, AttributeSpecFCast,
        AttributeSpecBGRA, AttributeSpecI, AttributeSpecD> && ...);

template<typename TupleT>
struct is_valid_tuple_of_attribute_specs : std::false_type {};

/*
Unfortunately, this stops the diagnostic here, without reporting on `valid_attribute_specs` failure.
So if the type of the tuple element is incorrect, you do not get the full diagnostic telling you so.
But I right now can't imagine how to unpack a tuple type pack from a concept otherwise.
*/
template<valid_attribute_specs ...AttribSpecTs>
struct is_valid_tuple_of_attribute_specs<std::tuple<AttribSpecTs...>> : std::true_type {};

template<typename TupleT>
concept valid_tuple_of_attribute_specs = is_valid_tuple_of_attribute_specs<TupleT>::value;

} // namespace detail

template<typename VertexT>
concept specializes_attribute_traits =
    // This crashes clangd, but the second concept validates the same thing so we skip this.
    // specialization_of<std::remove_cvref_t<decltype(attribute_traits<VertexT>::specs)>, std::tuple> &&
    detail::valid_tuple_of_attribute_specs<std::remove_cvref_t<decltype(attribute_traits<VertexT>::specs)>>;





} // namespace josh
