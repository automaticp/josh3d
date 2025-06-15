#pragma once
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "Region.hpp"
#include "RenderEngine.hpp"
#include "stages/primary/IDBufferStorage.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/types.h>




namespace josh {

/*
The "*" denotes textures that are not owned by the GBuffer.

    Depth*    [D] ...        // Shared from the main render target.
    Normals   [0] RGB8_SNorm // [-1, 1] world-space signed normalized.
    Albedo    [1] RGB8       // [0, 1] "linear" color.
    Specular  [2] R8         // [0, 1] specular factor.
    ObjectID* [3] R32UI      // [0, UINT_MAX], shared from IDBuffer.
*/
struct GBuffer
{
    [[nodiscard]] auto bind_draw()       noexcept { return _fbo->bind_draw(); }
    [[nodiscard]] auto bind_read() const noexcept { return _fbo->bind_read(); }

    // Accessors for compatibility and consistency.
    auto resolution()        const noexcept -> Extent2I { return _resolution; }
    auto depth_texture()     const noexcept -> RawTexture2D<> { return _depth;     }
    auto normals_texture()   const noexcept -> RawTexture2D<> { return _normals;   }
    auto albedo_texture()    const noexcept -> RawTexture2D<> { return _albedo;    }
    auto specular_texture()  const noexcept -> RawTexture2D<> { return _specular;  }
    auto object_id_texture() const noexcept -> RawTexture2D<> { return _object_id; }

    static constexpr auto iformat_normals  = InternalFormat::RGB8_SNorm;
    static constexpr auto iformat_albedo   = InternalFormat::RGB8;
    static constexpr auto iformat_specular = InternalFormat::R8; // TODO: Shininess? What shininess?
    static constexpr u32  slot_normals     = 0;
    static constexpr u32  slot_albedo      = 1;
    static constexpr u32  slot_specular    = 2;
    static constexpr u32  slot_object_id   = 3;

    // HMM: This could *technically* be 0, but it's hard to imagine why.
    // The worse case is when this is dangling instead. This shouldn't
    // happen within a frame, until a new frame starts so it's all OK
    // I guess, but we need to formalize this a bit.
    RawTexture2D<>    _depth;
    UniqueTexture2D   _normals;
    UniqueTexture2D   _albedo;
    UniqueTexture2D   _specular;
    RawTexture2D<>    _object_id;
    Extent2I          _resolution = { 0, 0 };
    UniqueFramebuffer _fbo;

    void _resize(Extent2I new_resolution);
    void _reset_depth(RawTexture2D<> new_depth);
    void _reset_object_id(RawTexture2D<> new_object_id);
};

inline void GBuffer::_resize(Extent2I new_resolution)
{
    if (_resolution == new_resolution) return;
    _resolution = new_resolution;
    _normals  = {};
    _albedo   = {};
    _specular = {};
    _normals ->allocate_storage(_resolution, iformat_normals);
    _albedo  ->allocate_storage(_resolution, iformat_albedo);
    _specular->allocate_storage(_resolution, iformat_specular);
    _fbo->attach_texture_to_color_buffer(_normals,  slot_normals);
    _fbo->attach_texture_to_color_buffer(_albedo,   slot_albedo);
    _fbo->attach_texture_to_color_buffer(_specular, slot_specular);
    _fbo->specify_color_buffers_for_draw(slot_normals, slot_albedo, slot_specular, slot_object_id);
}

inline void GBuffer::_reset_depth(RawTexture2D<> new_depth)
{
    _depth = new_depth;
    _fbo->attach_texture_to_depth_buffer(_depth);
}

inline void GBuffer::_reset_object_id(RawTexture2D<> new_object_id)
{
    _object_id = new_object_id;
    _fbo->attach_texture_to_color_buffer(_object_id, slot_object_id);
    // FIXME: Shouldn't this "disable" the slot if the texture is 0?
}


/*
Provides the storage for the GBuffer and clears it on each pass.

Place it before any other stages that draw into the GBuffer.
*/
struct GBufferStorage
{
    void operator()(RenderEnginePrimaryInterface& engine);

    GBuffer gbuffer;
};


inline void GBufferStorage::operator()(
    RenderEnginePrimaryInterface& engine)
{
    gbuffer._resize(engine.main_resolution());
    gbuffer._reset_depth(engine.main_depth_texture());
    if (auto* idbuffer = engine.belt().try_get<IDBuffer>())
        gbuffer._reset_object_id(idbuffer->object_id_texture());

    const BindGuard bfbo = gbuffer.bind_draw();

    glapi::clear_color_buffer(bfbo, GBuffer::slot_normals,  RGBAF{ 0.f, 0.f, 0.f });
    glapi::clear_color_buffer(bfbo, GBuffer::slot_albedo,   RGBAF{ 0.f, 0.f, 0.f });
    glapi::clear_color_buffer(bfbo, GBuffer::slot_specular, RGBAF{ 0.f });

    engine.belt().put_ref(gbuffer);
}


} // namespace josh::stages::primary
