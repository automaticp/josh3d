#pragma once
#include <string>
#include <array>
#include <stdexcept>
#include <utility>
#include <stb_image.h>
#include <glbinding/gl/gl.h>

#include "TypeAliases.h"
#include "ResourceAllocators.h"

using namespace gl;

class Texture : public TextureAllocator {
private:
	std::string filename_;
	const static std::array<GLenum, 32> texUnits_s;

	struct BasicImageData {
		unsigned char* data;
		int width;
		int height;
		int numChannels;
	};


public:
	explicit Texture(std::string filename, GLenum internalFormat = GL_RGBA, GLenum format = GL_NONE);

	void bind() { glBindTexture(GL_TEXTURE_2D, id_); }

	static void setActiveUnit(int texUnit) { glActiveTexture(texUnits_s.at(texUnit)); }

	// set Active Texture Unit and then bind the Texture to it
	void setActiveUnitAndBind(int texUnit) {
		setActiveUnit(texUnit);
		bind();
	}

	const std::string& getFilename() const { return filename_; }

private:
	BasicImageData loadTextureImage(int numDesiredChannels = 0);

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


inline Texture::Texture(std::string filename, GLenum internalFormat, GLenum format) :
		filename_{ std::move(filename) } {

	// FIXME: depending on the 'format', the numer of channels loaded can be coerced into the requested value
	//  with numDesiredChannels. Needs a map between 'format' and number of channels though.
	BasicImageData imageData { loadTextureImage(0) };

	if (format == GL_NONE) {
		switch (imageData.numChannels) {
			case 1: format = GL_RED; break;
			case 2: format = GL_RG; break;
			case 3: format = GL_RGB; break;
			case 4: format = GL_RGBA; break;
			default: format = GL_RED; // FIXME: what's a reasonable default behaviour?
		}
	}

	glBindTexture(GL_TEXTURE_2D, id_);
	glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat), imageData.width, imageData.height, 0,
		format, GL_UNSIGNED_BYTE, imageData.data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(imageData.data);
}

Texture::BasicImageData Texture::loadTextureImage(int numDesiredChannels) {

	BasicImageData imageData{};
	stbi_set_flip_vertically_on_load(true);

	static const std::string textureDir{ "resources/textures/" };
	std::string texturePath{ textureDir + filename_ };

	imageData.data = stbi_load(texturePath.c_str(), &imageData.width, &imageData.height, &imageData.numChannels, numDesiredChannels);
	if ( !imageData.data ) {
		throw std::runtime_error("runtime_error: could not read image file " + texturePath);
	}

	return imageData;
}


