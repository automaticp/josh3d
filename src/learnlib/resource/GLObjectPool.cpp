#include "GLObjectPool.hpp"
#include "DataPool.hpp"
#include "GLObjects.hpp"


namespace learn {


template<>
Shared<TextureHandle>
GLObjectPool<TextureHandle, DataPool<TextureData>>::load_data_from(const std::string& path) {
    Shared<TextureData> tex_data{ upstream_.load(path) };

    auto new_handle = std::make_shared<TextureHandle>();
    new_handle->bind().attach_data(*tex_data);

    return new_handle;
}




} // namespace learn
