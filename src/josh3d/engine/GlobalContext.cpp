#include "GlobalContext.hpp"
#include "DefaultTextures.hpp"


namespace josh::globals {


void init_all() {
    detail::init_default_textures();
}


void clear_all() {
    detail::clear_default_textures();
}


} // namespace josh::globals
