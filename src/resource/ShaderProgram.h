#pragma once
#include <utility>
#include <vector>
#include <exception>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "TypeAliases.h"
#include "Shader.h"
#include "ResourceAllocators.h"


class ShaderProgram : public ShaderProgramAllocator {
private:
	std::vector<refw<Shader>> shaders_;

public:
	explicit ShaderProgram(std::vector<refw<Shader>> shaders) : shaders_{ std::move(shaders) } {
		link();
#ifndef NDEBUG
		std::cerr << getLinkInfo();
#endif
	}

	void link() const;

	void use() const { glUseProgram(id_); }

	std::string getLinkInfo() const;

	GLint getLinkSuccess() const;

	GLint getUniformLocation(const std::string& name) const {
		return glGetUniformLocation(id_, name.c_str());
	}

	GLint getUniformLocation(const GLchar* name) const {
		return glGetUniformLocation(id_, name);
	}

	// this enables calls like: shaderProgram.setUniform("viewMat", viewMat);
	template<typename... Types>
	void setUniform(const GLchar* name, Types... args) const {
		ShaderProgram::setUniform(getUniformLocation(name), args...);
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


void ShaderProgram::link() const {
	for (Shader& shader: shaders_) {
		glAttachShader(id_, shader);
	}
	glLinkProgram(id_);

	GLint success = getLinkSuccess();
	if ( success != GL_TRUE ) {
		std::string linkInfo{ getLinkInfo() };
		throw std::runtime_error(std::string("runtime_error: program linking failed") + linkInfo);
	}
}

GLint ShaderProgram::getLinkSuccess() const {
	GLint success;
	glGetProgramiv(id_, GL_LINK_STATUS, &success);
	return success;
}

inline std::string ShaderProgram::getLinkInfo() const {
	std::string output{};
	GLint success = getLinkSuccess();
	output += "\nLinking Status: " + ((success == GL_TRUE) ? std::string("Success") : std::string("Failure"));
	output += "\nProgram Id: " + std::to_string(id_) + "\n";

	return output;
}

