#pragma once
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "RenderEngine.hpp"
#include "Region.hpp"


namespace josh {


struct IDBuffer
{
    [[nodiscard]] auto bind_draw()       noexcept { return _fbo->bind_draw(); }
    [[nodiscard]] auto bind_read() const noexcept { return _fbo->bind_read(); }

    auto resolution()        const noexcept -> Extent2I { return _object_id->get_resolution(); }
    auto object_id_texture() const noexcept -> RawTexture2D<> { return _object_id; }

    static constexpr auto iformat_object_id = InternalFormat::R32UI;
    static constexpr u32  slot_object_id    = 0; // In the internal FBO, this is rarely used.

    UniqueTexture2D   _object_id;
    UniqueFramebuffer _fbo;
    void _resize(Extent2I new_resolution);
};

inline void IDBuffer::_resize(Extent2I new_resolution)
{
    if (_object_id->get_resolution() == new_resolution) return;
    _object_id = {};
    _object_id->allocate_storage(new_resolution, iformat_object_id);
    _fbo->attach_texture_to_color_buffer(_object_id, 0);
}


} // namespace josh


namespace josh::stages::primary {


/*
Provides the storage for the ObjectID, resizes and clears it on each pass.

Place it before any other stages that draw into the IDBuffer.
*/
struct IDBufferStorage
{
    IDBuffer idbuffer;
    void operator()(RenderEnginePrimaryInterface& engine);
};


inline void IDBufferStorage::operator()(
    RenderEnginePrimaryInterface& engine)
{
    idbuffer._resize(engine.main_resolution());

    BindGuard bfbo = idbuffer.bind_draw();

    // The ObjectID buffer is cleared with the null sentinel value.
    constexpr auto null_color = to_underlying(nullent);
    glapi::clear_color_buffer(bfbo, 0, RGBAUI{ .r=null_color });

    engine.belt().put_ref(idbuffer);
}


} // namespace josh::stages::primary
