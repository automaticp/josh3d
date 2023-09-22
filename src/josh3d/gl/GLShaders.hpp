#pragma once
#include "AndThen.hpp"
#include "GLMutability.hpp" // IWYU pragma: keep (concepts)
#include "GLScalars.hpp"
#include "RawGLHandles.hpp"
#include "ULocation.hpp"
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>



namespace josh {


template<mutability_tag MutT> class RawShader;
template<mutability_tag MutT> class ActiveShaderProgram;
template<mutability_tag MutT> class RawShaderProgram;




template<mutability_tag MutT>
class RawShader
    : public RawShaderHandle<MutT>
    , public detail::ObjectHandleTypeInfo<RawShader, MutT>
{
public:
    using RawShaderHandle<MutT>::RawShaderHandle;

    void set_source(const GLchar* src) const
        requires gl_mutable<MutT>
    {
        gl::glShaderSource(this->id(), 1, &src, nullptr);
    }

    void compile() const {
        gl::glCompileShader(this->id());
    }
};






template<mutability_tag MutT>
class ActiveShaderProgram
    : public detail::AndThen<ActiveShaderProgram<MutT>>
{
private:
    friend RawShaderProgram<MutT>;
    RawShaderProgram<MutT> parent_;
    ActiveShaderProgram(RawShaderProgram<MutT> parent)
        : parent_{ parent }
    {}
public:
    template<typename... Args>
    ActiveShaderProgram& uniform(const GLchar* name, Args&&... args) {
        const auto location = location_of(name);
        ActiveShaderProgram::set_uniform(location, std::forward<Args>(args)...);
        return *this;
    }

    template<typename... Args>
    ActiveShaderProgram& uniform(ULocation location, Args&&... args) {
        ActiveShaderProgram::set_uniform(location, std::forward<Args>(args)...);
        return *this;
    }

    ULocation location_of(const GLchar* uniform_name) const {
        return gl::glGetUniformLocation(parent_, uniform_name);
    }





    // Values float
	static void set_uniform(ULocation location, GLfloat val0) {
		gl::glUniform1f(location, val0);
	}

    static void set_uniform(ULocation location, GLfloat val0, GLfloat val1) {
		gl::glUniform2f(location, val0, val1);
	}

    static void set_uniform(ULocation location, GLfloat val0, GLfloat val1, GLfloat val2) {
		gl::glUniform3f(location, val0, val1, val2);
	}

    static void set_uniform(ULocation location, GLfloat val0, GLfloat val1, GLfloat val2, GLfloat val3) {
		gl::glUniform4f(location, val0, val1, val2, val3);
	}

    // Values int
    static void set_uniform(ULocation location, GLint val0) {
		gl::glUniform1i(location, val0);
	}

    static void set_uniform(ULocation location, GLint val0, GLint val1) {
		gl::glUniform2i(location, val0, val1);
	}

    static void set_uniform(ULocation location, GLint val0, GLint val1, GLint val2) {
		gl::glUniform3i(location, val0, val1, val2);
	}

    static void set_uniform(ULocation location, GLint val0, GLint val1, GLint val2, GLint val3) {
		gl::glUniform4i(location, val0, val1, val2, val3);
	}

    // Values uint
    static void set_uniform(ULocation location, GLuint val0) {
		gl::glUniform1ui(location, val0);
	}

    static void set_uniform(ULocation location, GLuint val0, GLuint val1) {
		gl::glUniform2ui(location, val0, val1);
	}

    static void set_uniform(ULocation location, GLuint val0, GLuint val1, GLuint val2) {
		gl::glUniform3ui(location, val0, val1, val2);
	}

    static void set_uniform(ULocation location, GLuint val0, GLuint val1, GLuint val2, GLuint val3) {
		gl::glUniform4ui(location, val0, val1, val2, val3);
	}

    // Vector float
	static void set_uniform(ULocation location, const glm::vec1& v, GLsizei count = 1) {
		gl::glUniform1fv(location, count, glm::value_ptr(v));
	}

    static void set_uniform(ULocation location, const glm::vec2& v, GLsizei count = 1) {
		gl::glUniform2fv(location, count, glm::value_ptr(v));
	}

    static void set_uniform(ULocation location, const glm::vec3& v, GLsizei count = 1) {
		gl::glUniform3fv(location, count, glm::value_ptr(v));
	}

    static void set_uniform(ULocation location, const glm::vec4& v, GLsizei count = 1) {
		gl::glUniform4fv(location, count, glm::value_ptr(v));
	}

    // Matrix float
	static void set_uniform(ULocation location, const glm::mat2& m, GLsizei count = 1, GLboolean transpose = gl::GL_FALSE) {
		gl::glUniformMatrix2fv(location, count, transpose, glm::value_ptr(m));
	}

    static void set_uniform(ULocation location, const glm::mat3& m, GLsizei count = 1, GLboolean transpose = gl::GL_FALSE) {
		gl::glUniformMatrix3fv(location, count, transpose, glm::value_ptr(m));
	}

    static void set_uniform(ULocation location, const glm::mat4& m, GLsizei count = 1, GLboolean transpose = gl::GL_FALSE ) {
		gl::glUniformMatrix4fv(location, count, transpose, glm::value_ptr(m));
	}


    bool validate() const {
        gl::glValidateProgram(parent_);
        GLint is_valid;
        gl::glGetProgramiv(parent_, gl::GL_VALIDATE_STATUS, &is_valid);
        return is_valid;
    }

};




template<mutability_tag MutT>
class RawShaderProgram
    : public RawShaderProgramHandle<MutT>
    , public detail::ObjectHandleTypeInfo<RawShaderProgram, MutT>
{
public:
    using RawShaderProgramHandle<MutT>::RawShaderProgramHandle;

    void attach_shader(RawShader<GLConst> shader) const
        requires gl_mutable<MutT>
    {
        gl::glAttachShader(*this, shader);
    }

    void link() const {
        gl::glLinkProgram(*this);
    }

    ActiveShaderProgram<MutT> use() const {
        gl::glUseProgram(*this);
        return { *this };
    }

    ULocation location_of(const GLchar* name) const noexcept {
        return glGetUniformLocation(*this, name);
    }


    bool validate() {
        gl::glValidateProgram(*this);
        GLint is_valid;
        gl::glGetProgramiv(*this, gl::GL_VALIDATE_STATUS, &is_valid);
        return is_valid;
    }
};






} // namespace josh
