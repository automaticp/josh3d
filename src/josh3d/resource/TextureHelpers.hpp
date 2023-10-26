#pragma once
#include "GLScalars.hpp"
#include "GLTextures.hpp"
#include "GLMutability.hpp"
#include "TextureData.hpp"
#include "CubemapData.hpp"


namespace josh {


void attach_data_to_texture(BoundTexture2D<GLMutable>& tex,
    const TextureData& data, GLenum internal_format);

void attach_data_to_texture(BoundTexture2D<GLMutable>& tex,
    const TextureData& data, GLenum internal_format, GLenum format);

void attach_data_to_cubemap(BoundCubemap<GLMutable>& cube,
    const CubemapData& data, GLenum internal_format);

void attach_data_to_cubemap(BoundCubemap<GLMutable>& cube,
    const CubemapData& data, GLenum internal_format, GLenum format);


} // namespace josh
