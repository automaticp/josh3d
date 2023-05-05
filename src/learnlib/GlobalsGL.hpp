#pragma once
#include "Globals.hpp"
#include "GLObjects.hpp"
#include "GLObjectPool.hpp"
#include "TextureHandlePool.hpp"


namespace learn::globals {


extern TextureHandlePool texture_handle_pool;

extern Shared<TextureHandle> default_diffuse_texture;
extern Shared<TextureHandle> default_specular_texture;


}
