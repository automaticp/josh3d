#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "TypeAliases.h"
#include "IResource.h"
#include "Shader.h"


class ShaderProgramAllocator : public IResource {
protected:
	ShaderProgramAllocator() noexcept { id_ = glCreateProgram(); }

public:
	virtual ~ShaderProgramAllocator() override { glDeleteProgram(id_); }
};


class ShaderProgram : public IResource {
private:
	std::vector<refw<Shader>> shaders_;

	void acquireResource() noexcept { id_ = glCreateProgram(); }
	void releaseResource() noexcept { glDeleteProgram(id_); }

public:
	explicit ShaderProgram(const std::vector<refw<Shader>>& shaders);

	virtual ~ShaderProgram() override { releaseResource(); }

	std::string getLinkInfo();

	void use() const {
		glUseProgram(id_);
	}

	GLint getUniformLocation(const std::string& name) const {
		return glGetUniformLocation(id_, name.c_str());
	}

	GLint getUniformLocation(const GLchar* name) const {
		return glGetUniformLocation(id_, name);
	}


	// values float
	static void setUniform(int location, float val0) {
		glUniform1f(location, val0);
	}
	static void setUniform(int location, float val0, float val1) {
		glUniform2f(location, val0, val1);
	}
	static void setUniform(int location, float val0, float val1, float val2) {
		glUniform3f(location, val0, val1, val2);
	}
	static void setUniform(int location, float val0, float val1, float val2, float val3) {
		glUniform4f(location, val0, val1, val2, val3);
	}
	// values int
	static void setUniform(int location, int val0) {
		glUniform1i(location, val0);
	}
	static void setUniform(int location, int val0, int val1) {
		glUniform2i(location, val0, val1);
	}
	static void setUniform(int location, int val0, int val1, int val2) {
		glUniform3i(location, val0, val1, val2);
	}
	static void setUniform(int location, int val0, int val1, int val2, int val3) {
		glUniform4i(location, val0, val1, val2, val3);
	}
	// values uint
	static void setUniform(int location, unsigned int val0) {
		glUniform1ui(location, val0);
	}
	static void setUniform(int location, unsigned int val0, unsigned int val1) {
		glUniform2ui(location, val0, val1);
	}
	static void setUniform(int location, unsigned int val0, unsigned int val1, unsigned int val2) {
		glUniform3ui(location, val0, val1, val2);
	}
	static void setUniform(int location, unsigned int val0, unsigned int val1, unsigned int val2, unsigned int val3) {
		glUniform4ui(location, val0, val1, val2, val3);
	}
	// vector float
	static void setUniform(int location, const glm::vec1& v, GLsizei count = 1) {
		glUniform1fv(location, count, glm::value_ptr(v));
	}
	static void setUniform(int location, const glm::vec2& v, GLsizei count = 1) {
		glUniform2fv(location, count, glm::value_ptr(v));
	}
	static void setUniform(int location, const glm::vec3& v, GLsizei count = 1) {
		glUniform3fv(location, count, glm::value_ptr(v));
	}
	static void setUniform(int location, const glm::vec4& v, GLsizei count = 1) {
		glUniform4fv(location, count, glm::value_ptr(v));
	}
	// matrix float
	static void setUniform(int location, const glm::mat2& m, GLsizei count = 1, GLboolean transpose = GL_FALSE) {
		glUniformMatrix2fv(location, count, transpose, glm::value_ptr(m));
	}
	static void setUniform(int location, const glm::mat3& m, GLsizei count = 1, GLboolean transpose = GL_FALSE) {
		glUniformMatrix3fv(location, count, transpose, glm::value_ptr(m));
	}
	static void setUniform(int location, const glm::mat4& m, GLsizei count = 1, GLboolean transpose = GL_FALSE ) {
		glUniformMatrix4fv(location, count, transpose, glm::value_ptr(m));
	}

};


inline ShaderProgram::ShaderProgram(const std::vector<refw<Shader>>& shaders) :
		shaders_{ shaders } {

	acquireResource();
	for ( Shader& shader : shaders ) {
		glAttachShader(id_, shader);
	}
	glLinkProgram(id_);

	int success;
	glGetProgramiv(id_, GL_LINK_STATUS, &success);
	if ( !success ) {
		std::string linkInfo{ getLinkInfo() };
		// delete id before thorwing (undoes glCreateProgram())
		releaseResource();
		throw std::runtime_error(std::string("runtime_error: program linking failed") + linkInfo);
	}
}

inline std::string ShaderProgram::getLinkInfo() {
	std::string output{};
	int success;

	glGetProgramiv(id_, GL_LINK_STATUS, &success);
	output += "\nLinking Status: " + ((success == GL_TRUE) ? std::string("Success") : std::string("Failure"));
	output += "\nProgram Id: " + std::to_string(id_);

	return output;
}

