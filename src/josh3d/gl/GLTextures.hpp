#pragma once
#include "GLObjectHandles.hpp"
#include "GLScalars.hpp"
#include "AndThen.hpp"
#include "Size.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/types.h>




namespace josh {


class TextureData;
class CubemapData;


class Texture2D;      class BoundTexture2D;      class BoundConstTexture2D;
class Texture2DArray; class BoundTexture2DArray; class BoundConstTexture2DArray;
class Texture2DMS;    class BoundTexture2DMS;    class BoundConstTexture2DMS;
class Cubemap;        class BoundCubemap;        class BoundConstCubemap;
class CubemapArray;   class BoundCubemapArray;   class BoundConstCubemapArray;




/*
TODO: Figure out a way to return a const handle from const bind()
functions.

1. This doesn't work as returning by const value doesn't mean that
it can't initialize non-const lvalues. Thanks, C++.

    const BoundHandle bind() const { ... }

This is still legal:

    BoundHandle b = const_object.bind();


2. This doesn't work as the constness is not propagated
by inheritance.

    template<typename T>
    class ConstImpl : public T {};

    template<typename T>
    using Const = ConstImpl<const T>;

    Const<BoundHandle> bind() const { ... }

That is, non-const memfuns are still callable:

    const_object.bind().modify_stuff().unbind();

Trying to do

    template<typename T>
    using Const = const ConstImpl<T>;

brings you back to point number 1.

===
There are 2 options I have in mind that sort of work:

3. Defining a set of seperate ConstBoundHandle types with restricted functionality.

    BoundHandle      bind()       { ... }
    ConstBoundHandle bind() const { ... }

The downside is that you have to write a bunch more types. The idea in points 1 and 2 was
to generically create those types based on what's exposed in BoundHandles for const access.
But I so far can't seem to figure out a way to do that. Other than...

4. Return possibly const qualified references to some objects in global storage.

    // At global scope (possibly thread_local):
    namespace detail {
    BoundHandle global_bound_handle;
    }

    // As member functions:
    BoundHandle&       bind()       { return detail::global_bound_handle; }
    const BoundHandle& bind() const { return detail::global_bound_handle; }

If the bound handles are made non-copyable and non-movable (can be done with a trait),
then the access can be properly controlled with const qualifiers for memfuns
of bound handles.

This also can play into the overall OpenGL context tracking and switching,
if there's a need for this. Some bound handles will require storing some
state for utility reasons (ex. ActiveShaderProgram), and some others
have potentially more bind points that expected (ex. read and draw framebuffers
can be bound at the same time).

*/



/*
On the topic of what would make sense to actually consider to be a const operation:

1. Modification of a property of an OpenGL object specifically: writing to/resizing buffers,
changing draw hints, parameters, etc. - is a non-const operation.

2. Operation that modifies an OpenGL context but not the properties of objects: binding,
changing active units, buffer bindnigs, etc. - _can_ be considered a const operation.

3. Read operation on an object: getting properties, validation, etc - is a const operation.

The point 2 is probably the most important one, as without it, you can't really do anything
useful and still preserve some sense of const-correctness. If I can't even bind a texture
for sampling (reading) when it's const, then that const handle is useless for me.

Yes, you can still abuse this and find ways to write to the texture even when binding
a const handle, but, like, don't, ok? You can just write raw OpenGL if you want it that much.

*/

namespace detail {

template<typename CRTP, typename BoundT, typename BoundConstT, GLenum TargetV>
class BindableTexture {
public:
    static constexpr GLenum target_type = TargetV;

    BoundT bind() {
        gl::glBindTexture(TargetV, static_cast<CRTP&>(*this).id());
        return {};
    }

    BoundConstT bind() const {
        gl::glBindTexture(TargetV, static_cast<const CRTP&>(*this).id());
        return {};
    }


    BoundT bind_to_unit(GLenum tex_unit) {
        set_active_unit(tex_unit);
        return bind();
    }

    BoundConstT bind_to_unit(GLenum tex_unit) const {
        set_active_unit(tex_unit);
        return bind();
    }

    BoundT bind_to_unit_index(GLsizei tex_unit_index) {
        set_active_unit(gl::GLenum(gl::GL_TEXTURE0 + tex_unit_index));
        return bind();
    }

    BoundConstT bind_to_unit_index(GLsizei tex_unit_index) const {
        set_active_unit(GLenum(gl::GL_TEXTURE0 + tex_unit_index));
        return bind();
    }

    static void set_active_unit(GLenum tex_unit) {
        gl::glActiveTexture(tex_unit);
    }
};


template<typename CRTP, GLenum TargetV>
class SetParameter {
public:
    // Technically all applies to the Active Tex Unit
    // and not to the bound texture directly. But...

    CRTP& set_parameter(GLenum param_name, GLint param_value) {
        gl::glTexParameteri(TargetV, param_name, param_value);
        return as_self();
    }

    CRTP& set_parameter(GLenum param_name, GLenum param_value) {
        gl::glTexParameteri(TargetV, param_name, param_value);
        return as_self();
    }

    CRTP& set_parameter(GLenum param_name, GLfloat param_value) {
        gl::glTexParameterf(TargetV, param_name, param_value);
        return as_self();
    }

    CRTP& set_parameter(GLenum param_name, const GLfloat* param_values) {
        gl::glTexParameterfv(TargetV, param_name, param_values);
        return as_self();
    }

private:
    CRTP& as_self() noexcept {
        return static_cast<CRTP&>(*this);
    }
};




// These exist so that we could 'befriend' the right layer
// that actually constructs the bound handles.
//
// "Friendship is not inherited" - thanks C++.
// Makes it real easy to write implementation traits...
using BindableTexture2D =
    detail::BindableTexture<Texture2D, BoundTexture2D, BoundConstTexture2D, gl::GL_TEXTURE_2D>;

using BindableTexture2DArray =
    detail::BindableTexture<Texture2DArray, BoundTexture2DArray, BoundConstTexture2DArray, gl::GL_TEXTURE_2D_ARRAY>;

using BindableTexture2DMS =
    detail::BindableTexture<Texture2DMS, BoundTexture2DMS, BoundConstTexture2DMS, gl::GL_TEXTURE_2D_MULTISAMPLE>;

using BindableCubemap =
    detail::BindableTexture<Cubemap, BoundCubemap, BoundConstCubemap, gl::GL_TEXTURE_CUBE_MAP>;

using BindableCubemapArray =
    detail::BindableTexture<CubemapArray, BoundCubemapArray, BoundConstCubemapArray, gl::GL_TEXTURE_CUBE_MAP_ARRAY>;


} // namepsace detail









// I am writing extra const classes for now,
// until a better solution is mature enough.
class BoundConstTexture2D
    : public detail::AndThen<BoundConstTexture2D>
{
private:
    friend detail::BindableTexture2D;
    BoundConstTexture2D() = default;
public:
    static void unbind() {
        gl::glBindTexture(gl::GL_TEXTURE_2D, 0);
    }
};


class BoundTexture2D
    : public detail::AndThen<BoundTexture2D>
    , public detail::SetParameter<BoundTexture2D, gl::GL_TEXTURE_2D>
{
private:
    friend detail::BindableTexture2D;
    BoundTexture2D() = default;

public:
    static void unbind() {
        gl::glBindTexture(gl::GL_TEXTURE_2D, 0);
    }

    // FIXME: Technically is not part of OpenGL.
    // Make it a free function or responsibility of TextureData.
    BoundTexture2D& attach_data(const TextureData& tex_data,
        GLenum internal_format = gl::GL_RGBA, GLenum format = gl::GL_NONE);

    BoundTexture2D& specify_image(Size2I size,
        GLenum internal_format, GLenum format, GLenum type,
        const void* data, GLint mipmap_level = 0)
    {
        gl::glTexImage2D(
            gl::GL_TEXTURE_2D, mipmap_level, static_cast<GLint>(internal_format),
            size.width, size.height, 0, format, type, data
        );
        return *this;
    }

};


class Texture2D
    : public TextureHandle
    , public detail::BindableTexture2D
{};








class BoundConstTexture2DArray
    : public detail::AndThen<BoundConstTexture2DArray>
{
private:
    friend detail::BindableTexture2DArray;
    BoundConstTexture2DArray() = default;
public:
    static void unbind() { gl::glBindTexture(gl::GL_TEXTURE_2D_ARRAY, 0); }
};


class BoundTexture2DArray
    : public detail::AndThen<BoundTexture2DArray>
    , public detail::SetParameter<BoundTexture2DArray, gl::GL_TEXTURE_2D_ARRAY>
{
private:
    friend detail::BindableTexture2DArray;
    BoundTexture2DArray() = default;
public:
    static void unbind() {
        gl::glBindTexture(gl::GL_TEXTURE_2D_ARRAY, 0);
    }

    BoundTexture2DArray& specify_all_images(Size3I size,
        GLenum internal_format, GLenum format, GLenum type,
        const void* data, GLint mipmap_level = 0)
    {
        gl::glTexImage3D(
            gl::GL_TEXTURE_2D_ARRAY, mipmap_level, internal_format,
            size.width, size.height, size.depth, 0, format, type, data
        );
        return *this;
    }

};


class Texture2DArray
    : public TextureHandle
    , public detail::BindableTexture2DArray
{};








class BoundConstTexture2DMS
    : public detail::AndThen<BoundConstTexture2DMS>
{
private:
    friend detail::BindableTexture2DMS;
    BoundConstTexture2DMS() = default;

public:
    static void unbind() {
        gl::glBindTexture(gl::GL_TEXTURE_2D_MULTISAMPLE, 0);
    }
};


class BoundTexture2DMS
    : public detail::AndThen<BoundTexture2DMS>
    , public detail::SetParameter<BoundTexture2DMS, gl::GL_TEXTURE_2D_MULTISAMPLE>
{
private:
    friend detail::BindableTexture2DMS;
    BoundTexture2DMS() = default;

public:
    static void unbind() {
        gl::glBindTexture(gl::GL_TEXTURE_2D_MULTISAMPLE, 0);
    }

    BoundTexture2DMS& specify_image(Size2I size,
        GLsizei nsamples, GLenum internal_format = gl::GL_RGB,
        GLboolean fixed_sample_locations = gl::GL_TRUE)
    {
        gl::glTexImage2DMultisample(
            gl::GL_TEXTURE_2D_MULTISAMPLE, nsamples, internal_format,
            size.width, size.height, fixed_sample_locations
        );
        return *this;
    }

};


class Texture2DMS
    : public TextureHandle
    , public detail::BindableTexture2DMS
{};








class BoundConstCubemap
    : public detail::AndThen<BoundConstCubemap>
{
private:
    friend detail::BindableCubemap;
    BoundConstCubemap() = default;
public:
    static void unbind() {
        gl::glBindTexture(gl::GL_TEXTURE_CUBE_MAP, 0);
    }
};


class BoundCubemap
    : public detail::AndThen<BoundCubemap>
    , public detail::SetParameter<BoundCubemap, gl::GL_TEXTURE_CUBE_MAP>
{
private:
    friend detail::BindableCubemap;
    BoundCubemap() = default;
public:
    static void unbind() {
        gl::glBindTexture(gl::GL_TEXTURE_CUBE_MAP, 0);
    }

    BoundCubemap& specify_image(GLenum target,
        Size2I size, GLenum internal_format, GLenum format,
        GLenum type, const void* data, GLint mipmap_level = 0)
    {
        gl::glTexImage2D(
            target, mipmap_level, internal_format,
            size.width, size.height, 0, format, type, data
        );
        return *this;
    }

    BoundCubemap& specify_all_images(Size2I size,
        GLenum internal_format, GLenum format, GLenum type,
        const void* data, GLint mipmap_level = 0)
    {
        for (size_t i{ 0 }; i < 6; ++i) {
            specify_image(
                gl::GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                size, internal_format, format,
                type, data, mipmap_level
            );
        }
        return *this;
    }

    BoundCubemap& attach_data(const CubemapData& tex_data,
        GLenum internal_format = gl::GL_RGB, GLenum format = gl::GL_NONE);


};


class Cubemap
    : public TextureHandle
    , public detail::BindableCubemap
{
public:
    Cubemap() {
        using enum GLenum;
        // FIXME: This doesn't have to exist here.
        // If you want your skyboxes, do it yourself.
        this->bind()
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE)
            .unbind();
    }
};






class BoundConstCubemapArray
    : public detail::AndThen<BoundConstCubemapArray>
{
private:
    friend detail::BindableCubemapArray;
    BoundConstCubemapArray() = default;
public:
    static void unbind() {
        gl::glBindTexture(gl::GL_TEXTURE_CUBE_MAP_ARRAY, 0);
    }
};


class BoundCubemapArray
    : public detail::AndThen<BoundCubemapArray>
    , public detail::SetParameter<BoundCubemapArray, gl::GL_TEXTURE_CUBE_MAP_ARRAY>
{
private:
    friend detail::BindableCubemapArray;
    BoundCubemapArray() = default;
public:
    static void unbind() {
        gl::glBindTexture(gl::GL_TEXTURE_CUBE_MAP_ARRAY, 0);
    }

    BoundCubemapArray& specify_all_images(
        Size2I face_size, GLsizei num_cubemaps,
        GLenum internal_format, GLenum format, GLenum type,
        const void* data, GLint mipmap_level = 0)
    {
        gl::glTexImage3D(
            gl::GL_TEXTURE_CUBE_MAP_ARRAY, mipmap_level, internal_format,
            face_size.width, face_size.height, 6 * num_cubemaps, 0, format, type, data
        );
        return *this;
    }

};


class CubemapArray
    : public TextureHandle
    , public detail::BindableCubemapArray
{};



} // namespace josh
