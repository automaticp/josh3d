#pragma once
#include <concepts>
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/boolean.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/types.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include "AndThen.hpp"
#include "GLObjectAllocators.hpp"
#include "GlobalsUtil.hpp"
#include "ULocation.hpp"
#include "VertexTraits.hpp"


namespace learn {


/*
This file defines thin wrappers around various OpenGL objects: Buffers, Arrays, Textures, Shaders, etc.
Each GL object wrapper defines a 'Bound' dummy nested class that permits actions,
which are only applicable to bound or in-use GL objects.

Bound dummies do not perform any sanity checks for actually being bound or being used in correct context.
Their lifetimes do not end when the parent object is unbound.
Use-after-unbinding is still a programmer error.
It's recommended to use them as rvalues whenever possible; their methods support chaining.

The interface of Bound dummies serves as a guide for establising dependencies
between GL objects and correct order of calls to OpenGL API.

The common pattern for creating a Vertex Array (VAO) with a Vertex Buffer (VBO)
and an Element Buffer (EBO) attached in terms of these wrappes looks like this:

    VAO vao;
    VBO vbo;
    EBO ebo;
    auto bvao = vao.bind();
    vbo.bind().attach_data(...).associate_with(bvao, attribute_layout);
    ebo.bind(bvao).attach_data(...);
    bvao.unbind();

From the example above you can infer that the association between VAO and VBO is made
during the VBO::associate_with(...) call (glVertexAttribPointer(), in particular),
whereas the EBO is associated with a currently bound VAO when it gets bound itself.

The requirement to pass a reference to an existing VAO::Bound dummy during these calls
also implies their dependency on the currently bound Vertex Array. It would not make
sense to make these calls in absence of a bound VAO.
*/


class TextureData;
class CubemapData;

/*
In order to not move trivial single-line definitions into a .cpp file
and to not have to prepend every OpenGL type and function with gl::,
we're 'using namespace gl' inside of 'leaksgl' namespace,
and then reexpose the symbols back to this namespace at the end
with 'using leaksgl::Type' declarations.
*/

namespace leaksgl {

using namespace gl;

class VAO;
class VBO;
class EBO;
class BoundVAO;
class BoundVBO;
class BoundEBO;


// P.S. Some definitions were moved to a *.cpp file to break dependencies

class BoundVAO : public detail::AndThen<BoundVAO> {
private:
    friend class VAO;
    BoundVAO() = default;

public:
    BoundVAO& enable_array_access(GLuint attrib_index) {
        glEnableVertexAttribArray(attrib_index);
        return *this;
    }

    BoundVAO& disable_array_access(GLuint attrib_index) {
        glDisableVertexAttribArray(attrib_index);
        return *this;
    }

    BoundVAO& draw_arrays(GLenum mode, GLint first, GLsizei count) {
        glDrawArrays(mode, first, count);
        return *this;
    }

    BoundVAO& draw_elements(GLenum mode, GLsizei count, GLenum type,
                    const void* indices_buffer = nullptr) {
        glDrawElements(mode, count, type, indices_buffer);
        return *this;
    }

    BoundVAO& draw_arrays_instanced(GLenum mode, GLint first, GLsizei count, GLsizei instance_count) {
        glDrawArraysInstanced(mode, first, count, instance_count);
        return *this;
    }

    BoundVAO& draw_elements_instanced(GLenum mode, GLsizei elem_count, GLenum type, GLsizei instance_count, const void* indices_buffer = nullptr) {
        glDrawElementsInstanced(mode, elem_count, type, indices_buffer, instance_count);
        return *this;
    }

    template<size_t N>
    BoundVAO& set_many_attribute_params(
        const std::array<AttributeParams, N>& aparams) {

        for (const auto& ap : aparams) {
            set_attribute_params(ap);
            this->enable_array_access(ap.index);
        }
        return *this;
    }

    template<size_t N>
    BoundVAO& associate_with(const BoundVBO& vbo,
                        const std::array<AttributeParams, N>& aparams) {
        this->set_many_attribute_params(aparams);
        return *this;
    }

    template<typename VertexT>
    BoundVAO& associate_with(const BoundVBO& vbo) {
        this->set_many_attribute_params(learn::VertexTraits<VertexT>::aparams);
        return *this;
    }

    static void set_attribute_params(const AttributeParams& ap) {
        glVertexAttribPointer(
            ap.index, ap.size, ap.type, ap.normalized,
            ap.stride_bytes, reinterpret_cast<const void*>(ap.offset_bytes)
        );
    }

    static void unbind() {
        glBindVertexArray(0u);
    }
};


class VAO : public VAOAllocator {
public:
    BoundVAO bind() {
        glBindVertexArray(id_);
        return {};
    }
};





class BoundAbstractBuffer : public detail::AndThen<BoundAbstractBuffer> {
private:
    GLenum type_;

    friend class AbstractBuffer;
    BoundAbstractBuffer(GLenum type) : type_{ type } {}

public:
    template<typename T>
    BoundAbstractBuffer& attach_data(size_t size, const T* data, GLenum usage) {
        glBufferData(
            type_,
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data),
            usage
        );
        return *this;
    }

    template<typename T>
    BoundAbstractBuffer& sub_data(size_t size, size_t offset, const T* data) {
        glBufferSubData(
            type_,
            static_cast<GLintptr>(offset * sizeof(T)),
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data)
        );
        return *this;
    }

    template<typename T>
    BoundAbstractBuffer& get_sub_data(size_t size,
        size_t offset, T* data)
    {
        glGetBufferSubData(
            type_,
            static_cast<GLintptr>(offset * sizeof(T)),
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<void*>(data)
        );
        return *this;
    }



    void unbind() {
        glBindBuffer(type_, 0);
    }
};


class AbstractBuffer : public BufferAllocator {
public:
    BoundAbstractBuffer bind_as(GLenum type) {
        glBindBuffer(type, id_);
        return { type };
    }

    static void unbind_as(GLenum type) {
        glBindBuffer(type, 0);
    }
};


class BoundSSBO : public detail::AndThen<BoundSSBO> {
private:
    friend class SSBO;
    BoundSSBO() = default;

public:
    template<typename T>
    BoundSSBO& attach_data(size_t size, const T* data, GLenum usage) {
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data),
            usage
        );
        return *this;
    }

    template<typename T>
    BoundSSBO& sub_data(size_t size, size_t offset, const T* data) {
        glBufferSubData(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<GLintptr>(offset * sizeof(T)),
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data)
        );
        return *this;
    }

    template<typename T>
    BoundSSBO& get_sub_data(size_t size,
        size_t offset, T* data)
    {
        glGetBufferSubData(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<GLintptr>(offset * sizeof(T)),
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<void*>(data)
        );
        return *this;
    }


    static void unbind() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0u);
    }
};

class SSBO : public BufferAllocator {
public:
    BoundSSBO bind() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id_);
        return {};
    }

    BoundSSBO bind_to(GLuint binding_index) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_index, id_);
        return {};
    }
};



class BoundVBO : public detail::AndThen<BoundVBO>{
private:
    friend class VBO;
    BoundVBO() = default;

public:
    template<typename T>
    BoundVBO& attach_data(size_t size, const T* data, GLenum usage) {
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data),
            usage
        );
        return *this;
    }

    template<size_t N>
    BoundVBO& associate_with(BoundVAO& vao,
                        const std::array<AttributeParams, N>& aparams) {
        vao.associate_with(*this, aparams);
        return *this;
    }

    template<size_t N>
    BoundVBO& associate_with(BoundVAO&& vao,
            const std::array<AttributeParams, N>& aparams) {
        return this->associate_with(vao, aparams);
    }

    template<typename VertexT>
    BoundVBO& associate_with(BoundVAO& vao) {
        vao.associate_with<VertexT>(*this);
        return *this;
    }

    template<typename VertexT>
    BoundVBO& associate_with(BoundVAO&& vao) {
        return this->associate_with<VertexT>(vao);
    }



    static void unbind() {
        glBindBuffer(GL_ARRAY_BUFFER, 0u);
    }
};


class VBO : public BufferAllocator {
public:
    BoundVBO bind() {
        glBindBuffer(GL_ARRAY_BUFFER, id_);
        return {};
    }
};






class BoundEBO : public detail::AndThen<BoundEBO> {
private:
    friend class EBO;
    BoundEBO() = default;

public:
    template<typename T>
    BoundEBO& attach_data(size_t size, const T* data, GLenum usage) {
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data),
            usage
        );
        return *this;
    }

    static void unbind() {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);
    }
};


class EBO : public BufferAllocator {
public:
    BoundEBO bind(const BoundVAO& vao) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
        return {};
    }
};




class BoundFramebuffer : public detail::AndThen<BoundFramebuffer> {
private:
    GLenum target_;

    friend class Framebuffer;
    BoundFramebuffer(GLenum target) : target_{ target } {};

public:
    void unbind() {
        glBindFramebuffer(target_, 0);
    }

    static void unbind_as(GLenum target) {
        glBindFramebuffer(target, 0);
    }

    BoundFramebuffer& attach_texture(GLuint texture, GLenum attachment, GLint mipmap_level = 0) {
        glFramebufferTexture2D(target_, attachment, GL_TEXTURE_2D, texture, mipmap_level);
        return *this;
    }

    BoundFramebuffer& attach_multisample_texture(GLuint texture, GLenum attachment, GLint mipmap_level = 0) {
        glFramebufferTexture2D(target_, attachment, GL_TEXTURE_2D_MULTISAMPLE, texture, mipmap_level);
        return *this;
    }

    BoundFramebuffer& attach_renderbuffer(GLuint renderbuffer, GLenum attachment) {
        glFramebufferRenderbuffer(target_, attachment, GL_RENDERBUFFER, renderbuffer);
        return *this;
    }

    BoundFramebuffer& attach_cubemap(GLuint cubemap, GLenum attachment, GLint mipmap_level = 0) {
        glFramebufferTexture(target_, attachment, cubemap, mipmap_level);
        return *this;
    }

    BoundFramebuffer& attach_texture_layer(GLuint texture, GLenum attachment, GLint layer, GLint mipmap_level = 0) {
        glFramebufferTextureLayer(target_, attachment, texture, mipmap_level, layer);
        return *this;
    }

    // FIXME: might be a good idea to make 2 separate child classes for
    // DRAW and READ framebuffers and establish a blit_to() and blit_from()
    // methods with clear DRAW/READ bound state dependency. Something like:
    //
    // ... BoundReadFramebuffer::blit_to(BoundDrawFramebuffer& dst, ...);
    //
    BoundFramebuffer& blit(GLint src_x0, GLint src_y0, GLint src_x1, GLint src_y1,
        GLint dst_x0, GLint dst_y0, GLint dst_x1, GLint dst_y1,
        ClearBufferMask buffer_mask, GLenum interp_filter)
    {
        glBlitFramebuffer(
            src_x0, src_y0, src_x1, src_y1, dst_x0, dst_y0, dst_x1, dst_y1,
            buffer_mask, interp_filter
        );
        return *this;
    }
};


class Framebuffer : public FramebufferAllocator {
public:
    BoundFramebuffer bind() {
        return bind_as(GL_FRAMEBUFFER);
    }

    BoundFramebuffer bind_as(GLenum target) {
        glBindFramebuffer(target, id_);
        return { target };
    }
};




class BoundRenderbuffer : public detail::AndThen<BoundRenderbuffer> {
private:
    friend class Renderbuffer;
    BoundRenderbuffer() = default;

public:
    BoundRenderbuffer& create_storage(GLsizei width, GLsizei height, GLenum internal_format) {
        glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width, height);
        return *this;
    }

    BoundRenderbuffer& create_multisample_storage(GLsizei width, GLsizei height,
        GLsizei samples, GLenum internal_format)
    {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, internal_format, width, height);
        return *this;
    }

    static void unbind() {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
};



class Renderbuffer : public RenderbufferAllocator {
public:
    BoundRenderbuffer bind() {
        glBindRenderbuffer(GL_RENDERBUFFER, id_);
        return {};
    }

};






class BoundTextureHandle : public detail::AndThen<BoundTextureHandle> {
private:
    friend class TextureHandle;
    BoundTextureHandle() = default;

public:
    static void unbind() {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    BoundTextureHandle& attach_data(const TextureData& tex_data,
        GLenum internal_format = GL_RGBA, GLenum format = GL_NONE);

    BoundTextureHandle& specify_image(GLsizei width, GLsizei height,
        GLenum internal_format, GLenum format, GLenum type,
        const void* data, GLint mipmap_level = 0) {

        glTexImage2D(
            GL_TEXTURE_2D, mipmap_level, static_cast<GLint>(internal_format),
            width, height, 0, format, type, data
        );
        return *this;
    }

    // Technically applies to Active Tex Unit and not to the Bound Texture directly
    BoundTextureHandle& set_parameter(GLenum param_name, GLint param_value) {
        glTexParameteri(GL_TEXTURE_2D, param_name, param_value);
        return *this;
    }

    BoundTextureHandle& set_parameter(GLenum param_name, GLenum param_value) {
        glTexParameteri(GL_TEXTURE_2D, param_name, param_value);
        return *this;
    }

    BoundTextureHandle& set_parameter(GLenum param_name, GLfloat param_value) {
        glTexParameterf(GL_TEXTURE_2D, param_name, param_value);
        return *this;
    }

    BoundTextureHandle& set_parameter(GLenum param_name, const GLfloat* param_values) {
        glTexParameterfv(GL_TEXTURE_2D, param_name, param_values);
        return *this;
    }

};


class TextureHandle : public TextureAllocator {
public:
    BoundTextureHandle bind() {
        glBindTexture(GL_TEXTURE_2D, id_);
        return {};
    }

    BoundTextureHandle bind_to_unit(GLenum tex_unit) {
        set_active_unit(tex_unit);
        bind();
        return {};
    }

    static void set_active_unit(GLenum tex_unit) {
        glActiveTexture(tex_unit);
    }
};




class BoundTextureMS : public detail::AndThen<BoundTextureMS> {
private:
    friend class TextureMS;
    BoundTextureMS() = default;

public:
    static void unbind() {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    }

    BoundTextureMS& specify_image(GLsizei width, GLsizei height, GLsizei nsamples,
        GLenum internal_format = GL_RGB, GLboolean fixed_sample_locations = GL_TRUE)
    {
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, nsamples, internal_format, width, height, fixed_sample_locations);
        return *this;
    }

    BoundTextureMS& set_parameter(GLenum param_name, GLint param_value) {
        glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, param_name, param_value);
        return *this;
    }

    BoundTextureMS& set_parameter(GLenum param_name, GLenum param_value) {
        glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, param_name, param_value);
        return *this;
    }

    BoundTextureMS& set_parameter(GLenum param_name, GLfloat param_value) {
        glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, param_name, param_value);
        return *this;
    }

    BoundTextureMS& set_parameter(GLenum param_name, const GLfloat* param_values) {
        glTexParameterfv(GL_TEXTURE_2D_MULTISAMPLE, param_name, param_values);
        return *this;
    }


};


class TextureMS : public TextureAllocator {
public:
    BoundTextureMS bind() {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id_);
        return {};
    }

    BoundTextureMS bind_to_unit(GLenum tex_unit) {
        set_active_unit(tex_unit);
        bind();
        return {};
    }

    static void set_active_unit(GLenum tex_unit) {
        glActiveTexture(tex_unit);
    }
};





class BoundCubemap : public detail::AndThen<BoundCubemap> {
private:
    friend class Cubemap;
    BoundCubemap() = default;
public:
    static void unbind() {
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    BoundCubemap& specify_image(GLenum target, GLsizei width, GLsizei height,
        GLenum internal_format, GLenum format, GLenum type,
        const void* data, GLint mipmap_level = 0)
    {

        glTexImage2D(
            target, mipmap_level, internal_format,
            width, height, 0, format, type, data
        );
        return *this;
    }

    BoundCubemap& specify_all_images(GLsizei width, GLsizei height,
        GLenum internal_format, GLenum format, GLenum type,
        const void* data, GLint mipmap_level = 0)
    {
        for (size_t i{ 0 }; i < 6; ++i) {
            specify_image(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                width, height,
                internal_format, format,
                type, data, mipmap_level
            );
        }
        return *this;
    }


    BoundCubemap& set_parameter(GLenum param_name, GLint param_value) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, param_name, param_value);
        return *this;
    }

    BoundCubemap& set_parameter(GLenum param_name, GLenum param_value) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, param_name, param_value);
        return *this;
    }

    BoundCubemap& set_parameter(GLenum param_name, GLfloat param_value) {
        glTexParameterf(GL_TEXTURE_CUBE_MAP, param_name, param_value);
        return *this;
    }

    BoundCubemap& set_parameter(GLenum param_name, const GLfloat* param_values) {
        glTexParameterfv(GL_TEXTURE_CUBE_MAP, param_name, param_values);
        return *this;
    }


    BoundCubemap& attach_data(const CubemapData& tex_data,
        GLenum internal_format = GL_RGB, GLenum format = GL_NONE);


};




class Cubemap : public TextureAllocator {
public:
    Cubemap() {
        this->bind()
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE)
            .unbind();
    }

    BoundCubemap bind() {
        glBindTexture(GL_TEXTURE_CUBE_MAP, id_);
        return {};
    }

    BoundCubemap bind_to_unit(GLenum tex_unit) {
        set_active_unit(tex_unit);
        bind();
        return {};
    }

    static void set_active_unit(GLenum tex_unit) {
        glActiveTexture(tex_unit);
    }
};





class BoundCubemapArray : public detail::AndThen<BoundCubemapArray> {
private:
    friend class CubemapArray;
    BoundCubemapArray() = default;
public:
    static void unbind() {
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
    }

    BoundCubemapArray& specify_all_images(GLsizei width, GLsizei height, GLsizei depth,
        GLenum internal_format, GLenum format, GLenum type,
        const void* data, GLint mipmap_level = 0)
    {
        glTexImage3D(
            GL_TEXTURE_CUBE_MAP_ARRAY, mipmap_level, internal_format,
            width, height, 6 * depth, 0, format, type, data
        );
        return *this;
    }


    BoundCubemapArray& set_parameter(GLenum param_name, GLint param_value) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, param_name, param_value);
        return *this;
    }

    BoundCubemapArray& set_parameter(GLenum param_name, GLenum param_value) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, param_name, param_value);
        return *this;
    }

    BoundCubemapArray& set_parameter(GLenum param_name, GLfloat param_value) {
        glTexParameterf(GL_TEXTURE_CUBE_MAP_ARRAY, param_name, param_value);
        return *this;
    }

    BoundCubemapArray& set_parameter(GLenum param_name, const GLfloat* param_values) {
        glTexParameterfv(GL_TEXTURE_CUBE_MAP_ARRAY, param_name, param_values);
        return *this;
    }

};



class CubemapArray : public TextureAllocator {
public:
    BoundCubemapArray bind() {
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, id_);
        return {};
    }

    BoundCubemapArray bind_to_unit(GLenum tex_unit) {
        set_active_unit(tex_unit);
        bind();
        return {};
    }

    static void set_active_unit(GLenum tex_unit) {
        glActiveTexture(tex_unit);
    }
};














class Shader : public ShaderAllocator {
public:
    explicit Shader(GLenum type) : ShaderAllocator(type) {};

    Shader& set_source(const std::string& src) {
        const GLchar* csrc{ src.c_str() };
        glShaderSource(id_, 1, &csrc, nullptr);
        return *this;
    }

    Shader& set_source(const GLchar* src) {
        glShaderSource(id_, 1, &src, nullptr);
        return *this;
    }

    void compile() {
        glCompileShader(id_);
    }
};


class VertexShader : public Shader {
public:
    VertexShader() : Shader(GL_VERTEX_SHADER) {}
};


class FragmentShader : public Shader {
public:
    FragmentShader() : Shader(GL_FRAGMENT_SHADER) {}
};


class GeometryShader : public Shader {
public:
    GeometryShader() : Shader(GL_GEOMETRY_SHADER) {}
};


class ComputeShader : public Shader {
public:
    ComputeShader() : Shader(GL_COMPUTE_SHADER) {}
};




class ActiveShaderProgram : public detail::AndThen<ActiveShaderProgram> {
private:
    friend class ShaderProgram;
    // An exception to a common case of stateless Bound dummies.
    // In order to permit setting uniforms by name.
    GLuint parent_id_;
    ActiveShaderProgram(GLuint id) : parent_id_{ id } {}

public:
    bool validate();

    // This enables calls like: shader_program.uniform("viewMat", viewMat);
	template<typename... Args>
	ActiveShaderProgram& uniform(const GLchar* name, Args... args) {
		const auto location = location_of(name);
        // FIXME: Replace with something less
        // intrusive later.
        #ifndef NDEBUG
        if (location < 0) {
            globals::logstream <<
                "[Warning] Setting uniform " <<
                name << " at " << location << " location\n";
        }
        #endif
        ActiveShaderProgram::set_uniform(location, args...);
        return *this;
	}

    template<typename... Args>
	ActiveShaderProgram& uniform(const std::string& name, Args... args) {
        const auto location = location_of(name.c_str());
        #ifndef NDEBUG
        if (location < 0) {
            globals::logstream <<
                "[Warning] Setting uniform " <<
                name << " at " << location << " location\n";
        }
        #endif
		ActiveShaderProgram::set_uniform(location, args...);
        return *this;
	}

    template<typename... Args>
	ActiveShaderProgram& uniform(ULocation location, Args... args) {
		#ifndef NDEBUG
        if (location < 0) {
            globals::logstream << "[Warning] Setting uniform at -1 location\n";
        }
        #endif
        ActiveShaderProgram::set_uniform(location, args...);
        return *this;
	}

    ULocation location_of(const GLchar* uniform_name) const {
        return glGetUniformLocation(parent_id_, uniform_name);
    }

    ULocation location_of(const std::string& name) const {
        return glGetUniformLocation(parent_id_, name.c_str());
    }

    // Values float
	static void set_uniform(ULocation location, GLfloat val0) {
		glUniform1f(location, val0);
	}

    static void set_uniform(ULocation location, GLfloat val0, GLfloat val1) {
		glUniform2f(location, val0, val1);
	}

    static void set_uniform(ULocation location, GLfloat val0, GLfloat val1, GLfloat val2) {
		glUniform3f(location, val0, val1, val2);
	}

    static void set_uniform(ULocation location, GLfloat val0, GLfloat val1, GLfloat val2, GLfloat val3) {
		glUniform4f(location, val0, val1, val2, val3);
	}

    // Values int
    static void set_uniform(ULocation location, GLint val0) {
		glUniform1i(location, val0);
	}

    static void set_uniform(ULocation location, GLint val0, GLint val1) {
		glUniform2i(location, val0, val1);
	}

    static void set_uniform(ULocation location, GLint val0, GLint val1, GLint val2) {
		glUniform3i(location, val0, val1, val2);
	}

    static void set_uniform(ULocation location, GLint val0, GLint val1, GLint val2, GLint val3) {
		glUniform4i(location, val0, val1, val2, val3);
	}

    // Values uint
    static void set_uniform(ULocation location, GLuint val0) {
		glUniform1ui(location, val0);
	}

    static void set_uniform(ULocation location, GLuint val0, GLuint val1) {
		glUniform2ui(location, val0, val1);
	}

    static void set_uniform(ULocation location, GLuint val0, GLuint val1, GLuint val2) {
		glUniform3ui(location, val0, val1, val2);
	}

    static void set_uniform(ULocation location, GLuint val0, GLuint val1, GLuint val2, GLuint val3) {
		glUniform4ui(location, val0, val1, val2, val3);
	}

    // Vector float
	static void set_uniform(ULocation location, const glm::vec1& v, GLsizei count = 1) {
		glUniform1fv(location, count, glm::value_ptr(v));
	}

    static void set_uniform(ULocation location, const glm::vec2& v, GLsizei count = 1) {
		glUniform2fv(location, count, glm::value_ptr(v));
	}

    static void set_uniform(ULocation location, const glm::vec3& v, GLsizei count = 1) {
		glUniform3fv(location, count, glm::value_ptr(v));
	}

    static void set_uniform(ULocation location, const glm::vec4& v, GLsizei count = 1) {
		glUniform4fv(location, count, glm::value_ptr(v));
	}

    // Matrix float
	static void set_uniform(ULocation location, const glm::mat2& m, GLsizei count = 1, GLboolean transpose = GL_FALSE) {
		glUniformMatrix2fv(location, count, transpose, glm::value_ptr(m));
	}

    static void set_uniform(ULocation location, const glm::mat3& m, GLsizei count = 1, GLboolean transpose = GL_FALSE) {
		glUniformMatrix3fv(location, count, transpose, glm::value_ptr(m));
	}

    static void set_uniform(ULocation location, const glm::mat4& m, GLsizei count = 1, GLboolean transpose = GL_FALSE ) {
		glUniformMatrix4fv(location, count, transpose, glm::value_ptr(m));
	}

};


class ShaderProgram : public ShaderProgramAllocator {
public:
    ShaderProgram& attach_shader(GLuint shader) {
        glAttachShader(id_, shader);
        return *this;
    }

    ShaderProgram& link() {
        glLinkProgram(id_);
        return *this;
    }

    ActiveShaderProgram use() {
        glUseProgram(id_);
        return { id_ };
    }

    ULocation location_of(const std::string& name) const {
        return glGetUniformLocation(id_, name.c_str());
    }

    ULocation location_of(const GLchar* name) const {
        return glGetUniformLocation(id_, name);
    }

    static bool validate(GLuint program_id) {
        glValidateProgram(program_id);
        GLint is_valid;
        glGetProgramiv(program_id, GL_VALIDATE_STATUS, &is_valid);
        return is_valid;
    }

};


} // namespace leaksgl

using leaksgl::BoundAbstractBuffer, leaksgl::AbstractBuffer;
using leaksgl::BoundVAO, leaksgl::VAO;
using leaksgl::BoundVBO, leaksgl::VBO;
using leaksgl::BoundSSBO, leaksgl::SSBO;
using leaksgl::BoundEBO, leaksgl::EBO;
using leaksgl::BoundFramebuffer, leaksgl::Framebuffer;
using leaksgl::BoundRenderbuffer, leaksgl::Renderbuffer;
using leaksgl::BoundTextureHandle, leaksgl::TextureHandle;
using leaksgl::BoundTextureMS, leaksgl::TextureMS;
using leaksgl::BoundCubemap, leaksgl::Cubemap;
using leaksgl::BoundCubemapArray, leaksgl::CubemapArray;
using leaksgl::Shader, leaksgl::VertexShader, leaksgl::FragmentShader;
using leaksgl::ActiveShaderProgram, leaksgl::ShaderProgram;

} // namespace learn
