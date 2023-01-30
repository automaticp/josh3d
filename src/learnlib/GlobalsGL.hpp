#pragma once
#include "Globals.hpp"
#include "GLObjects.hpp"
#include "GLObjectPool.hpp"



namespace learn::globals {


extern GLObjectPool<TextureHandle> texture_handle_pool;

extern Shared<TextureHandle> default_diffuse_texture;
extern Shared<TextureHandle> default_specular_texture;


}
