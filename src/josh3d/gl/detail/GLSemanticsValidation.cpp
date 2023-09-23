#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include <glbinding/gl/enum.h>


/*
This exists for the sake of validating semantics of handle types:

- Whether certain expressions compile;
- Whether mutability is propagated correctly;
- Whether slicing works the way it should;

I'm not exactly sure of how to best do this. Could be with concepts and static_asserts,
could be by just trying to compile functions with internal linkage.

*How* is not important, really. Semantic validity is.

As such this file also serves as a way of documenting valid operations,
keeping track of bugs and incorrect behavior.

This is not unit tests, none of this is supposed to be executed at runtime.
I might write some actual runtime tests later.
*/


namespace josh::detail {
namespace {



template<template<typename> typename RawH, typename MutT>
concept can_be_unique =
    mutability_tag<MutT> &&
    requires { sizeof(GLUnique<RawH<MutT>>); };

static_assert(can_be_unique<RawTexture2D, GLMutable>);
static_assert(can_be_unique<RawTexture2D, GLConst>);

// FIXME:
// static_assert(can_be_unique<RawTextureHandle, GLMutable>);
// static_assert(can_be_unique<RawTextureHandle, GLConst>);


template<typename FromRawObj, typename ToRawObj>
concept conv_to_object =
    requires(FromRawObj from) { ToRawObj{ from }; };



[[maybe_unused]]
void sema_slicing_and_conversions() {

    // OK(+)       - Correct, should compile;
    // OK(x)       - Correct, should not compile;
    // WRONG(+)    - Incorrect, should compile, but does not;
    // WRONG(x)    - Incorrect, should not compile, but does;
    // CONFUSED(+) - Compiles correctly, but for the wrong reason;
    // CONFUSED(x) - Does not compile correctly, but for the wrong reason.

    {
        // Given unique object handles,
        GLUnique<RawTexture2D<GLMutable>> utm;
        GLUnique<RawTexture2D<GLConst>>   utc;

        // Can slice down to raw object handles;
        RawTexture2D<GLMutable>      rtm1{ utm }; // OK(+): GLMutable -> GLMutable
        RawTexture2D<GLConst>        rtc1{ utm }; // OK(+): GLMutable -> GLConst
    //  RawTexture2D<GLMutable>      rtm2{ utc }; // OK(x): GLConst   -x GLMutable
        RawTexture2D<GLConst>        rtc2{ utc }; // OK(+): GLConst   -> GLConst

        // Can slice down to raw kind handles;
        RawTextureHandle<GLMutable> rthm1{ utm }; // OK(+): GLMutable -> GLMutable
        RawTextureHandle<GLConst>   rthc1{ utm }; // OK(+): GLMutable -> GLConst
    //  RawTextureHandle<GLMutable> rthm2{ utc }; // OK(x): GLConst   -x GLMutable
        RawTextureHandle<GLConst>   rthc2{ utc }; // OK(+): GLConst   -> GLConst

        // Cannot slice down to other object handles;
    //  RawCubemap<GLMutable>       rcm1{ utm };  // CONFUSED(x): Ambiguous overload.
        RawCubemap<GLConst>         rcc1{ utm };  // WRONG(x): Implicit conversion to GLuint breaks this.
    //  RawCubemap<GLMutable>       rcm2{ utc };  // CONFUSED(x): Call to deleted constructor. Should just not exist.
    //  RawCubemap<GLConst>         rcc2{ utc };  // CONFUSED(x): Ambiguous overload.

        // Cannot slice down to other kind handles;
    //  RawBufferHandle<GLMutable>  rbm1{ utm };  // CONFUSED(x): Amibguous overload.
        RawBufferHandle<GLConst>    rbc1{ utm };  // WRONG(x): Implicit conversion.
    //  RawBufferHandle<GLMutable>  rbm2{ utc };  // CONFUSED(x): Call to deleted constructor. Should just not exist.
    //  RawBufferHandle<GLConst>    rbc2{ utc };  // CONFUSED(x): Amibguous overload.
    }


    {
        // Given raw object handles,
        RawTexture2D<GLMutable>     rtm{ GLuint{ 42 } };
        RawTexture2D<GLConst>       rtc{ GLuint{ 42 } };

        // Can slice down to raw kind handles;
        RawTextureHandle<GLMutable> rthm1{ rtm }; // OK(+): GLMutable -> GLMutable
        RawTextureHandle<GLConst>   rthc1{ rtm }; // OK(+): GLMutable -> GLConst
    //  RawTextureHandle<GLMutable> rthm2{ rtc }; // OK(x): GLConst   -x GLMutable
        RawTextureHandle<GLConst>   rthc2{ rtc }; // OK(+): GLConst   -> GLConst

        // Cannot slice down to other kind handles;
    //  RawBufferHandle<GLMutable>  rbm1{ rtm };  // CONFUSED(x): Amibguous overload.
        RawBufferHandle<GLConst>    rbc1{ rtm };  // WRONG(x): Implicit conversion.
    //  RawBufferHandle<GLMutable>  rbm2{ rtc };  // CONFUSED(x): Call to deleted constructor. Should just not exist.
    //  RawBufferHandle<GLConst>    rbc2{ rtc };  // CONFUSED(x): Amibguous overload.

    }


    {
        // Given raw kind handles,
        RawTextureHandle<GLMutable> rthm{ GLuint{ 42 } };
        RawTextureHandle<GLConst>   rthc{ GLuint{ 42 } };

        // Cannot slice down to other kind handles;
    //  RawBufferHandle<GLMutable>  rbm1{ rthm };  // CONFUSED(x): Amibguous overload.
        RawBufferHandle<GLConst>    rbc1{ rthm };  // WRONG(x): Implicit conversion.
    //  RawBufferHandle<GLMutable>  rbm2{ rthc };  // CONFUSED(x): Call to deleted constructor. Should just not exist.
    //  RawBufferHandle<GLConst>    rbc2{ rthc };  // CONFUSED(x): Amibguous overload.

        // Cannot convert to unrelated object handles;
    //  RawVBO<GLMutable>           rvbm1{ rthm };  // CONFUSED(x): Amibguous overload.
        RawVBO<GLConst>             rvbc1{ rthm };  // WRONG(x): Implicit conversion.
    //  RawVBO<GLMutable>           rvbm2{ rthc };  // CONFUSED(x): Call to deleted constructor. Should just not exist.
    //  RawVBO<GLConst>             rvbc2{ rthc };  // CONFUSED(x): Amibguous overload.
    }


    {
        // Given just an OpenGL id,
        GLuint id{ 42 };

        // Raw kind handle c-tor is explicit;
        RawTextureHandle<GLConst> rth1{ id }; // OK(+)
    //  RawTextureHandle<GLConst> rth2 = id;  // OK(x)

        // Raw object handle c-tor is explicit;
        RawTexture2D<GLConst> rt1{ id }; // OK(+)
    //  RawTexture2D<GLConst> rt2 = id;  // OK(x)

    }

}


[[maybe_unused]]
void sema_binding() {

    using enum GLenum;

    // OK(+)       - Correct, should compile;
    // OK(x)       - Correct, should not compile;
    // WRONG(+)    - Incorrect, should compile, but does not;
    // WRONG(x)    - Incorrect, should not compile, but does;
    // CONFUSED(+) - Compiles correctly, but for the wrong reason;
    // CONFUSED(x) - Does not compile correctly, but for the wrong reason.

    {
        // Given Texture2D handles,
        RawTexture2D<GLMutable> rtm{ 10 };
        RawTexture2D<GLConst>   rtc{ 11 };

        RawTexture2D<GLMutable>::size_type size{ 0, 0 };
        RawTexture2D<GLMutable>::spec_type spec{ GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE };

        // Bind-unbind, whatever, works;
        rtm.bind()
            .specify_image(size, spec, nullptr)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            .generate_mipmaps() // OK(+)
            .and_then([] {})
            .and_then([](BoundTexture2D<GLMutable>&) {})
            .unbind(); // OK(+)

        rtc.bind()
        //  .specify_image(size, spec, nullptr)
        //  .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)
        //  .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
        //  .generate_mipmaps() // OK(x)
            .and_then([] {})
            .and_then([](BoundTexture2D<GLConst>&) {})
            .unbind(); // OK(+)
    }


    {
        // Given VBO handles,
        RawVBO<GLMutable> rvbm{ 10 };
        RawVBO<GLConst>   rvbc{ 10 };

        // Bind-unbind, whatever;
        rvbm.bind()
            .specify_data<int>(1, nullptr, GL_STATIC_DRAW)
            .sub_data<int>(1, 0, nullptr) // OK(+)
            .get_sub_data<int>(1, 0, nullptr)
            .and_then([] {})
            .and_then([](BoundVBO<GLMutable>&) {})
            .unbind(); // OK(+)

        rvbc.bind()
        //  .specify_data<int>(1, nullptr, GL_STATIC_DRAW)
        //  .sub_data<int>(1, 0, nullptr) // OK(x)
            .get_sub_data<int>(1, 0, nullptr)
            .and_then([] {})
            .and_then([](BoundVBO<GLConst>&) {})
            .unbind(); // OK(+)
    }


    {
        // Given SSBO handles (indexed),
        RawSSBO<GLMutable> rsbm{ 10 };
        RawSSBO<GLConst>   rsbc{ 10 };

        // Bind-unbind, indexed, whatever;
        rsbm.bind_to_index(0)
            .specify_data<int>(1, nullptr, GL_STATIC_DRAW)
            .sub_data<int>(1, 0, nullptr) // OK(+)
            .get_sub_data<int>(1, 0, nullptr)
            .and_then([] {})
            .and_then([](BoundIndexedSSBO<GLMutable>&) {})
        //  .and_then([](BoundSSBO<GLMutable>&) {}) // OK(x)
            .unbind(); // OK(+)

        rsbc.bind_to_index(0)
        //  .specify_data<int>(1, nullptr, GL_STATIC_DRAW)
        //  .sub_data<int>(1, 0, nullptr) // OK(x)
            .get_sub_data<int>(1, 0, nullptr)
            .and_then([] {})
            .and_then([](BoundIndexedSSBO<GLConst>&) {}) // OK(+)
        //  .and_then([](BoundSSBO<GLMutable>&) {}) // OK(x)
            .unbind();
    }


    {


    }

}


} // namespace
} // namespace josh::detail
