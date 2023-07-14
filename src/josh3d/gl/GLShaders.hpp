#pragma once
#include "GLObjectHandles.hpp"
#include "AndThen.hpp"
#include "ULocation.hpp"
#include "GlobalsUtil.hpp"
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>


namespace josh {


/*
In order to not move trivial single-line definitions into a .cpp file
and to not have to prepend every OpenGL type and function with gl::,
we're 'using namespace gl' inside of 'leaksgl' namespace,
and then reexpose the symbols back to this namespace at the end
with 'using leaksgl::Type' declarations.
*/

namespace leaksgl {

using namespace gl;











class Shader : public ShaderHandle {
public:
    explicit Shader(GLenum type) : ShaderHandle(type) {};

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

    // This enables calls like:
    //   shader_program.uniform("viewMat", viewMat);
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
	ActiveShaderProgram& uniform(const std::string& name,
        Args... args)
    {
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


class ShaderProgram : public ShaderProgramHandle {
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


inline bool ActiveShaderProgram::validate() {
    return ShaderProgram::validate(parent_id_);
}






} // namespace leaksgl

using leaksgl::Shader;
using leaksgl::VertexShader, leaksgl::FragmentShader;
using leaksgl::ComputeShader, leaksgl::GeometryShader;
using leaksgl::ActiveShaderProgram, leaksgl::ShaderProgram;

} // namespace josh
