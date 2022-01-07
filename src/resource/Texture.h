#pragma once
#include <string>
#include <stdexcept>
#include <stb_image.h>

#include "TypeAliases.h"
#include "IResource.h"


class TextureAllocator : public IResource {
protected:
	TextureAllocator() noexcept {
		glGenTextures(1, &id_);
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] "
		          << "TextureAllocator()";
#endif
	}

public:
	virtual ~TextureAllocator() override {
#ifndef NDEBUG
		std::cerr << "\n[id: " << id_ << "] " << "~TextureAllocator()" ;
#endif
		glDeleteTextures(1, &id_);
	}
};


class Texture : public TextureAllocator {
private:
	std::string filename_;
	const static std::array<GLenum, 32> texUnits_s;

public:
	Texture(const std::string& filename, GLenum format, GLint internalFormat = GL_RGB);

	void bind() { glBindTexture(GL_TEXTURE_2D, id_); }

	static void setActiveUnit(int texUnit) { glActiveTexture(texUnits_s.at(texUnit)); }

	// set Active Texture Unit and then bind the Texture to it
	void setActiveUnitAndBind(int texUnit) {
		setActiveUnit(texUnit);
		bind();
	}

	const std::string& getFilename() const { return filename_; }


};


inline const std::array<GLenum, 32> Texture::texUnits_s{
	GL_TEXTURE0,  GL_TEXTURE1,  GL_TEXTURE2,  GL_TEXTURE3,  GL_TEXTURE4,
	GL_TEXTURE5,  GL_TEXTURE6,  GL_TEXTURE7,  GL_TEXTURE8,  GL_TEXTURE9,
	GL_TEXTURE10, GL_TEXTURE11, GL_TEXTURE12, GL_TEXTURE13, GL_TEXTURE14,
	GL_TEXTURE15, GL_TEXTURE16, GL_TEXTURE17, GL_TEXTURE18, GL_TEXTURE19,
	GL_TEXTURE20, GL_TEXTURE21, GL_TEXTURE22, GL_TEXTURE23, GL_TEXTURE24,
	GL_TEXTURE25, GL_TEXTURE26, GL_TEXTURE27, GL_TEXTURE28, GL_TEXTURE29,
	GL_TEXTURE30, GL_TEXTURE31 
};


inline Texture::Texture(const std::string& filename, GLenum format, GLint internalFormat) :
		filename_{ filename } {

	int width, height, numChannels;
	stbi_set_flip_vertically_on_load(true);

	static const std::string textureDir{ "resources/textures/" };
	std::string texturePath{ textureDir + filename };

	unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &numChannels, 0);
	if ( !data ) {
		throw std::runtime_error("runtime_error: could not read image file " + texturePath);
	}

	glBindTexture(GL_TEXTURE_2D, id_);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);
}


