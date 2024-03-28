#include "GLBuffers.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLFenceSync.hpp"
#include "GLFramebuffer.hpp"
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



namespace josh::dsa {
namespace {




// Buffers


[[maybe_unused]]
void buffer_operations() noexcept {

    UniqueBuffer<float> buf;

    buf.allocate_storage(
        NumElems{ 1 },
        StorageMode::StaticServer,
        PermittedMapping::ReadWrite);

    auto mapped = buf.map_range_for_write(
        OffsetElems{ 0 },
        NumElems{ 1 },
        PendingOperations::SynchronizeOnMap,
        FlushPolicy::MustFlushExplicity,
        PreviousContents::InvalidateMappedRange,
        Persistence::NotPersistent);

    do {

        mapped[0] = 1.f;
        buf.flush_mapped_range(OffsetElems{ 0 }, NumElems{ 1 });

    } while(!buf.unmap_current());


    RawUntypedBuffer<> ubuf = buf;

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
    RawProgram<> p{ 902 };
    // RawVertexShader<> sh{ 99 };
    // p.attach_shader(sh);
    p.uniform(Location{ 0 }, 1);
    p.uniform(Location{ 0 }, 0);
    glm::vec3 v{};
    // p.uniform("color", glm::vec3{ 1.f, 0.f, 1.f });
    // p.uniform("color", v);
    p.uniform("", 1.f);
}






// Textures


[[maybe_unused]]
inline void foooo(Layer layer) noexcept {
    static_cast<GLint>(layer);
}


[[maybe_unused]]
void texture_operations() {
    RawTexture2D<GLMutable> tex{ 32 };
    tex.set_sampler_wrap_all(Wrap::ClampToEdge);
    tex.set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Linear);
    RawTexture2D<GLConst> ct{ tex };
    RawTexture2DMS<GLMutable> tms{ 12 };
    tms.allocate_storage({ 1, 1 }, InternalFormat::RGBA8, NumSamples{ 4 }, SampleLocations::Fixed);
    // tms.allocate_storage({1, 1}, TexSpecMS{{}, {}, {}});
    // tex.allocate_storage({1, 1}, { enum_cast<PixelInternalFormat>(gl::GL_SIGNED_RGB_UNSIGNED_ALPHA_NV) });
    // tex.allocate_storage(Size2I{1, 1}, TexSpec{ PixelInternalFormat::Compressed_SRGBA_BPTC_UNorm });
    tex.allocate_storage({ 1, 1 }, InternalFormat::Compressed_SRGBA_BPTC_UNorm, NumLevels{ 7 });
    // pixel::RGBA arr[4];
    // tex.upload_image_region({ { 0, 0 }, { 2, 2 } }, arr);

    RawTexture2DArray<> t2darr{ 732 };
    t2darr.allocate_storage({ 16, 16 }, 32, InternalFormat::RGBA32F, NumLevels{ 5 });
    RawTexture3D<GLMutable> t3d{ 9203 };
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
    RawTexture2D t1{ 1 };
    RawTexture2D<GLMutable> t2{ 2 };
    t2 = t1;
    tex.set_sampler_wrap_s(Wrap::ClampToBorder);
    tex.set_sampler_wrap_all(Wrap::Repeat);


    RawCubemapArray<> c{ 9 };
    c.allocate_storage({ 64, 64 }, 6, InternalFormat::RGBA8, NumLevels{ 5 });

    {
        RawTexture2D t2d{ 2 };
        RawTexture3D t3d{ 3 };
        t2d.copy_image_region_to({}, { 512, 512 }, t3d, { 0, 0, 8 });
        t3d.copy_image_region_to({}, { 64,  64  }, t2d, { 0, 0 });
        RawTextureBuffer<> bu{ 9 };
    }

    RawTextureBuffer<> buft{ 3 };
    RawTextureRectangle<> rect{ 1 };
    rect.set_sampler_min_mag_filters(MinFilterNoLOD::Linear, MagFilter::Linear);



    {
        UniqueTexture2D tex;
        tex.allocate_storage({ 1024, 1024 }, InternalFormat::RGB16F, NumLevels{ 1 });
        tex.set_sampler_min_mag_filters(MinFilter::Linear, MagFilter::Linear);

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
    RawFramebuffer<> fb{  99 };
    fb.blit_to(fb, { {}, { 100, 100 } }, { {}, { 200, 200 } }, BufferMask::ColorBit | BufferMask::DepthBit, BlitFilter::Linear);
    RawDefaultFramebuffer<> dfb;
    dfb.specify_default_buffers_for_draw(DefaultFramebufferBuffer::BackLeft, DefaultFramebufferBuffer::BackRight);
    RawTexture2D<> tx{ 90 };
    RawTexture2DArray<> txa{ 99 };
    RawTexture2DMS<> txms{ 90 };
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




} // namespace
} // namespace josh::dsa {
