#pragma once
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include "GLObjectAllocators.hpp"
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

class BoundVAO {
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

    static void set_attribute_params(const AttributeParams& ap) {
        glVertexAttribPointer(
            ap.index, ap.size, ap.type, ap.normalized,
            ap.stride_bytes, reinterpret_cast<const void*>(ap.offset_bytes)
        );
    }

    void unbind() {
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





class BoundAbstractBuffer {
private:
    GLenum type_;

    friend class AbstractBuffer;
    BoundAbstractBuffer(GLenum type) : type_{ type } {}

public:
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

    void unbind_as(GLenum type) {
        glBindBuffer(type, 0);
    }
};





class BoundVBO {
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
        return associate_with(vao, aparams);
    }

    void unbind() {
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






class BoundEBO {
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

    void unbind() {
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






class BoundFramebuffer {
private:
    friend class Framebuffer;
    BoundFramebuffer() = default;

public:
    void unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    BoundFramebuffer& attach_texture(GLuint texture, GLenum attachment, GLint mipmap_level = 0) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, mipmap_level);
        return *this;
    }

    BoundFramebuffer& attach_renderbuffer(GLuint renderbuffer, GLenum attachment) {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderbuffer);
        return *this;
    }
};


class Framebuffer : public FramebufferAllocator {
public:
    BoundFramebuffer bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, id_);
        return {};
    }

};




class BoundRenderbuffer {
private:
    friend class Renderbuffer;
    BoundRenderbuffer() = default;

public:
    BoundRenderbuffer& create_storage(GLsizei width, GLsizei height, GLenum internal_format) {
        glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width, height);
        return *this;
    }

    void unbind() {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
};



class Renderbuffer : public RenderbufferAllocator {
public:
    void bind() {
        glBindRenderbuffer(GL_RENDERBUFFER, id_);
    }

};








class BoundTextureHandle {
private:
    friend class TextureHandle;
    BoundTextureHandle() = default;

public:
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





class ShaderProgram;


class ActiveShaderProgram {
private:
    friend class ShaderProgram;
    // An exception to a common case of stateless Bound dummies.
    // In order to permit setting uniforms by name.
    GLuint parent_id_;
    ActiveShaderProgram(GLuint id) : parent_id_{ id } {}

public:
    // This enables calls like: shaderProgram.setUniform("viewMat", viewMat);
	template<typename... Args>
	ActiveShaderProgram& uniform(const GLchar* name, Args... args) {
		ActiveShaderProgram::uniform(glGetUniformLocation(parent_id_, name), args...);
        return *this;
	}

    template<typename... Args>
	ActiveShaderProgram& uniform(const std::string& name, Args... args) {
		ActiveShaderProgram::uniform(glGetUniformLocation(parent_id_, name.c_str()), args...);
        return *this;
	}

    template<typename... Args>
	ActiveShaderProgram& uniform(GLint location, Args... args) {
		ActiveShaderProgram::uniform(location, args...);
        return *this;
	}


    // Values float
	static void uniform(int location, float val0) {
		glUniform1f(location, val0);
	}

    static void uniform(int location, float val0, float val1) {
		glUniform2f(location, val0, val1);
	}

    static void uniform(int location, float val0, float val1, float val2) {
		glUniform3f(location, val0, val1, val2);
	}

    static void uniform(int location, float val0, float val1, float val2, float val3) {
		glUniform4f(location, val0, val1, val2, val3);
	}

    // Values int
    static void uniform(int location, int val0) {
		glUniform1i(location, val0);
	}

    static void uniform(int location, int val0, int val1) {
		glUniform2i(location, val0, val1);
	}

    static void uniform(int location, int val0, int val1, int val2) {
		glUniform3i(location, val0, val1, val2);
	}

    static void uniform(int location, int val0, int val1, int val2, int val3) {
		glUniform4i(location, val0, val1, val2, val3);
	}

    // Values uint
    static void uniform(int location, unsigned int val0) {
		glUniform1ui(location, val0);
	}

    static void uniform(int location, unsigned int val0, unsigned int val1) {
		glUniform2ui(location, val0, val1);
	}

    static void uniform(int location, unsigned int val0, unsigned int val1, unsigned int val2) {
		glUniform3ui(location, val0, val1, val2);
	}

    static void uniform(int location, unsigned int val0, unsigned int val1, unsigned int val2, unsigned int val3) {
		glUniform4ui(location, val0, val1, val2, val3);
	}

    // Vector float
	static void uniform(int location, const glm::vec1& v, GLsizei count = 1) {
		glUniform1fv(location, count, glm::value_ptr(v));
	}

    static void uniform(int location, const glm::vec2& v, GLsizei count = 1) {
		glUniform2fv(location, count, glm::value_ptr(v));
	}

    static void uniform(int location, const glm::vec3& v, GLsizei count = 1) {
		glUniform3fv(location, count, glm::value_ptr(v));
	}

    static void uniform(int location, const glm::vec4& v, GLsizei count = 1) {
		glUniform4fv(location, count, glm::value_ptr(v));
	}

    // Matrix float
	static void uniform(int location, const glm::mat2& m, GLsizei count = 1, GLboolean transpose = GL_FALSE) {
		glUniformMatrix2fv(location, count, transpose, glm::value_ptr(m));
	}

    static void uniform(int location, const glm::mat3& m, GLsizei count = 1, GLboolean transpose = GL_FALSE) {
		glUniformMatrix3fv(location, count, transpose, glm::value_ptr(m));
	}

    static void uniform(int location, const glm::mat4& m, GLsizei count = 1, GLboolean transpose = GL_FALSE ) {
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

    GLint uniform_location(const std::string& name) const {
        return glGetUniformLocation(id_, name.c_str());
    }

    GLint uniform_location(const GLchar* name) const {
        return glGetUniformLocation(id_, name);
    }

};


} // namespace leaksgl

using leaksgl::BoundAbstractBuffer, leaksgl::AbstractBuffer;
using leaksgl::BoundVAO, leaksgl::VAO;
using leaksgl::BoundVBO, leaksgl::VBO;
using leaksgl::BoundEBO, leaksgl::EBO;
using leaksgl::BoundFramebuffer, leaksgl::Framebuffer;
using leaksgl::BoundRenderbuffer, leaksgl::Renderbuffer;
using leaksgl::BoundTextureHandle, leaksgl::TextureHandle;
using leaksgl::Shader, leaksgl::VertexShader, leaksgl::FragmentShader;
using leaksgl::ActiveShaderProgram, leaksgl::ShaderProgram;

} // namespace learn
