#pragma once
#include <string>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <concepts>
#include <memory>
#include <glad/glad.h>
#include "TypeAliases.h"
#include "IResource.h"
// TODO: make a base class for Id holders
// with m_id and m_isDeleted, typecasts to Id_t and bool, and virtual del() method

class Shader : public IResource {
private:
	std::string m_filename;
	GLenum m_type;

protected:
	virtual void acquireResource() override { m_id = glCreateShader(m_type); }
	virtual void releaseResource() override { glDeleteShader(m_id); }

public:
	Shader(GLenum type, const std::string& filename) :
		m_type{ type }, m_filename{ filename } {
		
		// check shader type to be a valid enum
		if ( (type != GL_VERTEX_SHADER) && (type != GL_FRAGMENT_SHADER) ) {
			throw std::invalid_argument("invalid_argument: invalid shader type");
		}

		// create shader id and compile from file
		acquireResource();
		std::string shaderSourceString{ getShaderFileSource(filename) };
		const char* shaderSource{ shaderSourceString.c_str() };
		glShaderSource(m_id, 1, &shaderSource, nullptr);
		glCompileShader(m_id);
		
		// check for succesful compilation
		int success;
		glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);
		if ( !success ) {
			std::string compileInfo{ getCompileInfo() };
			// delete id before thorwing (undoes glCreateShader())
			glDeleteShader(m_id);
			throw std::runtime_error(std::string("runtime_error: shader compilation failed") + compileInfo);
		}
	}

	GLenum getType() const noexcept { return m_type; }
	const std::string& getFilename() const noexcept { return m_filename; }
	std::string getSource() const { return getShaderFileSource(m_filename); }

	static std::string getShaderFileSource(const std::string& filename) {
		
		static const std::string shaderDir{ "resources/shaders/" };
		
		std::ifstream file{ shaderDir + filename };
		std::string outString{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
		return outString;
	}

	std::string getCompileInfo() {
		std::string output{};
		int shaderType, success, sourceLength;
		glGetShaderiv(m_id, GL_SHADER_TYPE, &shaderType);
		glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);
		glGetShaderiv(m_id, GL_SHADER_SOURCE_LENGTH, &sourceLength);
		
		std::string shaderTypeName{};
		if ( shaderType == GL_VERTEX_SHADER ) { shaderTypeName = "Vertex"; }
		else if ( shaderType == GL_FRAGMENT_SHADER ) { shaderTypeName = "Fragment"; }
		else { shaderTypeName = "???"; }

		output += "\nShader Id: " + std::to_string(m_id);
		output += "\nShader Type: " + shaderTypeName;
		output += "\nCompilation Status: " + ((success == GL_TRUE) ? std::string("Success") : std::string("Failure"));
		output += "\nSource File: " + m_filename;
		output += "\nSource Length: " + std::to_string(sourceLength);
		if ( success != GL_TRUE ) {
			int infoLength;
			glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &infoLength);
			auto infoLog{ std::make_unique<char[]>(infoLength) };
			glGetShaderInfoLog(m_id, infoLength, nullptr, infoLog.get());
			output += "\nInfo Length: " + std::to_string(infoLength);
			output += "\nInfo Message:< " + std::string(infoLog.get());
			output += " >";
		}
		output += '\n';
		return output;
	}

};


class FragmentShader : 
	public Shader {
public:
	explicit FragmentShader(const std::string& filename) :
		Shader(GL_FRAGMENT_SHADER, filename) {}
};


class VertexShader :
	public Shader {
public:
	explicit VertexShader(const std::string& filename) :
		Shader(GL_VERTEX_SHADER, filename) {}
};
