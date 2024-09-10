#include "GlobalContext.hpp"
#include "DefaultTextures.hpp"
#include "ShaderPool.hpp"


namespace josh::globals {


void init_all() {
    init_thread_local_shader_pool();
    detail::init_default_textures();
}


void clear_all() {
    clear_thread_local_shader_pool();
    detail::clear_default_textures();
}


} // namespace josh::globals
