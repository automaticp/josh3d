#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "TypeAliases.h"
#include "IResource.h"
#include "Shader.h"

class ShaderProgram : public IResource {
private:
	std::vector<refw<Shader>> m_shaders;

protected:
	void acquireResource() { m_id = glCreateProgram(); }
	void releaseResource() { glDeleteProgram(m_id); }

public:
	ShaderProgram(std::vector<refw<Shader>> shaders) :
		m_shaders{ shaders } {
		
		acquireResource();
		for ( Shader& shader : shaders ) {
			glAttachShader(m_id, shader);
		}
		glLinkProgram(m_id);
		
		int success;
		glGetProgramiv(m_id, GL_LINK_STATUS, &success);
		if ( !success ) {
			std::string linkInfo{ getLinkInfo() };
			// delete id before thorwing (undoes glCreateProgram())
			releaseResource();
			throw std::runtime_error(std::string("runtime_error: program linking failed") + linkInfo);
		}		
	}

	std::string getLinkInfo() {
		std::string output{};
		int success;

		glGetProgramiv(m_id, GL_LINK_STATUS, &success);
		output += "\nLinking Status: " + ((success == GL_TRUE) ? std::string("Success") : std::string("Failure"));
		output += "\nProgram Id: " + std::to_string(m_id);

		return output;
	}

	void use() const {
		glUseProgram(m_id);
	}

	int getUniformLocation(const std::string& name) const {
		return glGetUniformLocation(m_id, name.c_str());
	}

	int getUniformLocation(const char* name) const {
		return glGetUniformLocation(m_id, name);
	}


	// reddit said this is actually good
	// values float
	void setUniform(int location, float val0) const {
		glUniform1f(location, val0);
	}
	void setUniform(int location, float val0, float val1) const {
		glUniform2f(location, val0, val1);
	}
	void setUniform(int location, float val0, float val1, float val2) const {
		glUniform3f(location, val0, val1, val2);
	}
	void setUniform(int location, float val0, float val1, float val2, float val3) const {
		glUniform4f(location, val0, val1, val2, val3);
	}
	// values int
	void setUniform(int location, int val0) const {
		glUniform1i(location, val0);
	}
	void setUniform(int location, int val0, int val1) const {
		glUniform2i(location, val0, val1);
	}
	void setUniform(int location, int val0, int val1, int val2) const {
		glUniform3i(location, val0, val1, val2);
	}
	void setUniform(int location, int val0, int val1, int val2, int val3) const {
		glUniform4i(location, val0, val1, val2, val3);
	}
	// values uint
	void setUniform(int location, unsigned int val0) const {
		glUniform1ui(location, val0);
	}
	void setUniform(int location, unsigned int val0, unsigned int val1) const {
		glUniform2ui(location, val0, val1);
	}
	void setUniform(int location, unsigned int val0, unsigned int val1, unsigned int val2) const {
		glUniform3ui(location, val0, val1, val2);
	}
	void setUniform(int location, unsigned int val0, unsigned int val1, unsigned int val2, unsigned int val3) const {
		glUniform4ui(location, val0, val1, val2, val3);
	}
	// vector float
	void setUniform(int location, const glm::vec1& v, GLsizei count = 1) const {
		glUniform1fv(location, count, glm::value_ptr(v));
	}
	void setUniform(int location, const glm::vec2& v, GLsizei count = 1) const {
		glUniform2fv(location, count, glm::value_ptr(v));
	}
	void setUniform(int location, const glm::vec3& v, GLsizei count = 1) const {
		glUniform3fv(location, count, glm::value_ptr(v));
	}
	void setUniform(int location, const glm::vec4& v, GLsizei count = 1) const {
		glUniform4fv(location, count, glm::value_ptr(v));
	}
	// matrix float
	void setUniform(int location, const glm::mat2& m, GLsizei count = 1, GLboolean transpose = GL_FALSE) const {
		glUniformMatrix2fv(location, count, transpose, glm::value_ptr(m));
	}
	void setUniform(int location, const glm::mat3& m, GLsizei count = 1, GLboolean transpose = GL_FALSE) const {
		glUniformMatrix3fv(location, count, transpose, glm::value_ptr(m));
	}
	void setUniform(int location, const glm::mat4& m, GLsizei count = 1, GLboolean transpose = GL_FALSE ) const {
		glUniformMatrix4fv(location, count, transpose, glm::value_ptr(m));
	}





};

