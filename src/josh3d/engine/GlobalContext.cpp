#include "GlobalContext.hpp"
#include "DataPool.hpp"
#include "DefaultResources.hpp"
#include "TextureHandlePool.hpp"
#include "GLObjects.hpp"


namespace josh::globals {


void init_all() {
    detail::init_default_textures();
    detail::init_mesh_primitives();
}


void clear_all() {
    texture_data_pool.clear();
    texture_handle_pool.clear();
    detail::reset_default_textures();
    detail::reset_mesh_primitives();
}


} // namespace josh::globals
