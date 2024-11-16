#include "GLBuffers.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLFenceSync.hpp"
#include "GLFramebuffer.hpp"
#include "GLKind.hpp"
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "GLSampler.hpp"
#include "GLTextures.hpp"
#include "GLQueries.hpp"
#include "GLShaders.hpp"
#include "GLProgram.hpp"
#include "GLVertexArray.hpp"
#include "GLAllocator.hpp"
#include "GLUnique.hpp"
#include "GLAPICore.hpp"
#include "GLObjectHelpers.hpp"
#include <array>



namespace josh {
namespace {




// Buffers


[[maybe_unused]]
void buffer_operations() noexcept {

    UniqueBuffer<float> buf;

    buf->allocate_storage(1, { .mode = StorageMode::StaticServer, .mapping = PermittedMapping::ReadWrite });

    const MappingWritePolicies policies{
        PendingOperations::SynchronizeOnMap,
        FlushPolicy::MustFlushExplicity,
        PreviousContents::InvalidateMappedRange,
        Persistence::NotPersistent
    };
    const BufferRange range{ OffsetElems{ 0 }, NumElems{ 1 } };
    auto mapped = buf->map_range_for_write(range, policies);

    do {

        mapped[0] = 1.f;
        buf->flush_mapped_range({ OffsetElems{ 0 }, NumElems{ 1 } });

    } while (!buf->unmap_current());



    RawUntypedBuffer<> ubuf = buf.get();

}





} // namespace
// template<>
// struct uniform_traits<glm::vec3> {
//     static void set(RawShaderProgram<> program, Location location, const glm::vec3& v) noexcept {
//         program.set_uniform_vec3v(location, 1, glm::value_ptr(v));
//     }
// };

// template<size_t S>
// struct uniform_traits<GLint, GLfloat[S]> {
//     static void set(RawProgram<> program, Location location, GLsizei count, const GLfloat* data) noexcept {
//         program.set_uniform_floatv(location, count, data);
//     }
// };
namespace {


[[maybe_unused]]
void program_operations() {
    auto p = RawProgram<>::from_id(GLAllocator<GLKind::Program>::request());
    // RawVertexShader<> sh{ 99 };
    // p.attach_shader(sh);
    p.uniform(Location{ 0 }, 1);
    p.uniform(Location{ 0 }, 0);
    glm::vec3 v{};
    // p.uniform("color", glm::vec3{ 1.f, 0.f, 1.f });
    // p.uniform("color", v);
    p.uniform("", 1.f);
    GLAllocator<GLKind::Program>::release(p.id());
}






// Textures


[[maybe_unused]]
inline void foooo(Layer layer) noexcept {
    static_cast<GLint>(layer);
}


[[maybe_unused]]
void texture_operations() {
    auto tex = RawTexture2D<GLMutable>::from_id(32);
    tex.set_sampler_wrap_all(Wrap::ClampToEdge);
    tex.set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Linear);
    RawTexture2D<GLConst> ct{ tex };
    auto tms = RawTexture2DMS<GLMutable>::from_id(12);
    tms.allocate_storage({ 1, 1 }, InternalFormat::RGBA8, NumSamples{ 4 }, SampleLocations::Fixed);
    // tms.allocate_storage({1, 1}, TexSpecMS{{}, {}, {}});
    // tex.allocate_storage({1, 1}, { enum_cast<PixelInternalFormat>(gl::GL_SIGNED_RGB_UNSIGNED_ALPHA_NV) });
    // tex.allocate_storage(Size2I{1, 1}, TexSpec{ PixelInternalFormat::Compressed_SRGBA_BPTC_UNorm });
    tex.allocate_storage({ 1, 1 }, InternalFormat::Compressed_SRGBA_BPTC_UNorm, NumLevels{ 7 });
    // pixel::RGBA arr[4];
    // tex.upload_image_region({ { 0, 0 }, { 2, 2 } }, arr);

    auto t2darr = RawTexture2DArray<>::from_id(732);
    t2darr.allocate_storage({ 16, 16 }, 32, InternalFormat::RGBA32F, NumLevels{ 5 });
    auto t3d = RawTexture3D<GLMutable>::from_id(9203);
    t3d.allocate_storage({ 12, 23, 2 }, InternalFormat::RGBA8, NumLevels{ 7 });
    t3d.invalidate_image_region({ { 0, 0, 0 }, { 1, 1, 1 } }, MipLevel{ 6 });
    // t3d.upload_image_region({ {}, { 12, 23, 2 } }, arr, MipLevel{ 0 });
    t3d.generate_mipmaps();
    tex.bind_to_readonly_image_unit(ImageUnitFormat::RGBA16, 0, MipLevel{ 0 });
    // tex.bind_layer_to_read_image_unit(0, {}, 0);
    t3d.bind_layer_to_readonly_image_unit(Layer{ 0 }, ImageUnitFormat::RGBA8, 0);

    tex.set_sampler_compare_ref_depth_to_texture(true);
    tex.set_sampler_compare_func(CompareOp::NotEqual);

    // UniqueTexture2D tex{};
    // pixel::RGBA pixel_data[16 * 16];
    // tex.allocate_storage({ 512, 512 }, { PixelInternalFormat::Compressed_RGBA_BPTC_UNorm }, 7);
    // tex.fill_image(pixel::RGBAF{ 1.f, 0.f, 0.f, 1.f });

    // tex.upload_image_region({ { 127, 127 }, { 16, 16 } }, pixel_data);
    tex.generate_mipmaps();

    tex.set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Nearest);
    tex.set_sampler_wrap_all(Wrap::ClampToEdge);

    tex.bind_to_texture_unit(0);
    auto t1 = RawTexture2D<>::from_id(1);
    auto t2 = RawTexture2D<GLMutable>::from_id(2);
    t2 = t1;
    tex.set_sampler_wrap_s(Wrap::ClampToBorder);
    tex.set_sampler_wrap_all(Wrap::Repeat);


    auto c = RawCubemapArray<>::from_id(9);
    c.allocate_storage({ 64, 64 }, 6, InternalFormat::RGBA8, NumLevels{ 5 });

    {
        auto t2d = RawTexture2D<>::from_id(2);
        auto t3d = RawTexture3D<>::from_id(3);
        t2d.copy_image_region_to({}, { 512, 512 }, t3d, { 0, 0, 8 });
        t3d.copy_image_region_to({}, { 64,  64  }, t2d, { 0, 0 });
    }

    auto buft = RawTextureBuffer<>::from_id(3);
    auto rect = RawTextureRectangle<>::from_id(1);
    rect.set_sampler_min_mag_filters(MinFilterNoLOD::Linear, MagFilter::Linear);



    {
        UniqueTexture2D tex;
        tex->allocate_storage({ 1024, 1024 }, InternalFormat::RGB16F, NumLevels{ 1 });
        tex->set_sampler_min_mag_filters(MinFilter::Linear, MagFilter::Linear);

        auto tms = RawTexture2DMSArray<>::from_id(0);
        tms.allocate_storage({ 1024, 1024 }, 12, InternalFormat::RGBA8, NumSamples{ 4 }, SampleLocations::NotFixed);

    }

    max_num_levels({ 4096, 4096, 4096 }).value;
    // n.value;
    allocate_texture<TextureTarget::Texture3D>(Size3I{ 0, 0, 0 }, InternalFormat::RGBA, NumLevels{ 7 });


}


// Samplers

[[maybe_unused]]
void sampler_operations() {
    auto s = RawSampler<>::from_id(0);

}


// Framebuffer


[[maybe_unused]]
void framebuffer_operations() {
    auto fb = RawFramebuffer<>::from_id(99);
    fb.blit_to(fb, { {}, { 100, 100 } }, { {}, { 200, 200 } }, BufferMask::ColorBit | BufferMask::DepthBit, BlitFilter::Linear);
    RawDefaultFramebuffer<> dfb;
    dfb.specify_default_buffers_for_draw(DefaultFramebufferBuffer::BackLeft, DefaultFramebufferBuffer::BackRight);
    auto tx   = RawTexture2D<>::from_id(90);
    auto txa  = RawTexture2DArray<>::from_id(99);
    auto txms = RawTexture2DMS<>::from_id(91);
    fb.attach_texture_to_color_buffer(tx, 0, MipLevel{ 0 });
    fb.attach_texture_to_color_buffer(txms, 1);
    fb.attach_texture_to_stencil_buffer(tx);
    fb.attach_texture_layer_to_color_buffer(txa, Layer{ 3 }, 1, MipLevel{ 0 });

    // glapi::enable(Capability::ScissorTest);
}



} // namespace
// struct MalformedVertex {
//     float          xyz[3];
// };


// template<> struct attribute_traits<MalformedVertex> {
//     using specs_type = std::tuple<
//         AttributeTypeF
//     >;

//     static constexpr specs_type specs{
//         AttributeTypeF::Float
//     };
// };
namespace {



// Vertex Arrays

[[maybe_unused]]
void vertex_array_operations() {
    auto vao = RawVertexArray<>::from_id(9);
    vao.specify_float_attribute             (AttributeIndex{ 0 }, AttributeTypeF::Float,    AttributeComponents::RGB);
    vao.specify_integer_attribute           (AttributeIndex{ 1 }, AttributeTypeI::UInt,     AttributeComponents::Red);
    vao.specify_float_attribute_normalized  (AttributeIndex{ 2 }, AttributeTypeNorm::UByte, AttributeComponents::RGBA);
    vao.associate_attribute_with_buffer_slot(AttributeIndex{ 0 }, VertexBufferSlot{ 0 });
    vao.associate_attribute_with_buffer_slot(AttributeIndex{ 1 }, VertexBufferSlot{ 0 });
    vao.associate_attribute_with_buffer_slot(AttributeIndex{ 2 }, VertexBufferSlot{ 0 });

    auto buf = RawBuffer<float>::from_id(0);


    // vao.specify_custom_attributes<MalformedVertex>(AttributeIndex{ 0 });
    // vao.specify_custom_attributes<PackedVertex>(AttributeIndex{ 2 });

    vao.specify_float_attribute(
        AttributeIndex{ 0 }, AttributeTypeF::Float, AttributeComponents::RGBA, OffsetBytes{ 0 }
    );

    vao.attach_vertex_buffer(
        VertexBufferSlot{ 0 }, buf, OffsetBytes{ 0 }, StrideBytes{ 0 }
    );

    vao.associate_attribute_with_buffer_slot(
        AttributeIndex{ 0 }, VertexBufferSlot{ 0 }
    );

    vao.enable_attribute(
        AttributeIndex{ 0 }
    );
}






// GLShared

static void gimeraw(const RawVertexArray<GLMutable>&& vao, RawVertexArray<GLConst> cvao) {}

[[maybe_unused]]
static void glshared_operations() {

    GLShared<RawVertexArray<GLConst>> cvao;
    SharedVertexArray vao;
    vao->get_attached_element_buffer_id();
    gimeraw(vao, cvao);
    const auto& base_ref [[maybe_unused]] = static_cast<const RawVertexArray<>&>(vao);
    // gimeraw(vao, SharedVertexArray()); // This is slicing, this is baaaaad.
}


} // namespace
} // namespace josh
