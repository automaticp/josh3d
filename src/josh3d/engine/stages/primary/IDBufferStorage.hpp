#pragma once
#include "Attachments.hpp"
#include "GLTextures.hpp"
#include "RenderEngine.hpp"
#include "RenderTarget.hpp"
#include "SharedStorage.hpp"
#include "Size.hpp"


namespace josh {


class IDBuffer {
public:

    IDBuffer(const Size2I& resolution)
        : target_{
            resolution,
            { object_id_iformat }
        }
    {}

    [[nodiscard]] BindToken<Binding::DrawFramebuffer> bind_draw()       noexcept { return target_.bind_draw(); }
    [[nodiscard]] BindToken<Binding::ReadFramebuffer> bind_read() const noexcept { return target_.bind_read(); }

    auto object_id_attachment() const noexcept
        -> const ShareableAttachment<Renderable::Texture2D>&
    {
        return target_.color_attachment();
    }

    auto share_object_id_attachment() noexcept
        -> SharedAttachment<Renderable::Texture2D>
    {
        return target_.share_color_attachment();
    }

    auto object_id_texture() const noexcept
        -> RawTexture2D<GLConst>
    {
        return object_id_attachment().texture();
    }

    Size2I resolution() const noexcept { return target_.resolution(); }
    void resize(const Size2I& new_resolution) { target_.resize(new_resolution); }


private:
    using Target = RenderTarget<
        NoDepthAttachment,
        ShareableAttachment<Renderable::Texture2D>
    >;

    Target target_;

    static constexpr auto object_id_iformat = InternalFormat::R32UI;

};


} // namespace josh




namespace josh::stages::primary {


/*
Provides the storage for the ObjectID, resizes and clears it on each pass.

Place it before any other stages that draw into the IDBuffer.
*/
class IDBufferStorage {
public:
    IDBufferStorage(const Size2I& resolution)
        : idbuffer_{ resolution }
    {}

    auto share_write_view()       noexcept -> SharedStorageMutableView<IDBuffer> { return idbuffer_.share_mutable_view(); }
    auto share_read_view()  const noexcept -> SharedStorageView<IDBuffer>        { return idbuffer_.share_view(); }
    auto view_id_buffer()   const noexcept -> const IDBuffer&                    { return *idbuffer_; }
    void resize(const Size2I& new_resolution)                                    { idbuffer_->resize(new_resolution); }

    void operator()(RenderEnginePrimaryInterface& engine);

private:
    SharedStorage<IDBuffer> idbuffer_;

};




inline void IDBufferStorage::operator()(
    RenderEnginePrimaryInterface& engine)
{
    resize(engine.main_resolution());

    // The ObjectID buffer is cleared with the null sentinel value.
    constexpr entt::id_type null_color{ entt::null };

    BindGuard bound_fbo{ idbuffer_->bind_draw() };

    glapi::clear_color_buffer(bound_fbo, 0, RGBAUI{ .r=null_color });

}


} // namespace josh::stages::primary
