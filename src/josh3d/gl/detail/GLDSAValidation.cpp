#pragma once
#include "GLDSABuffers.hpp"
#include "GLDSAFenceSync.hpp"
#include "GLDSAFramebuffer.hpp"
#include "GLDSATextures.hpp"
#include "GLDSAQueries.hpp"
#include "GLDSAShaders.hpp"
#include "GLDSAShaderProgram.hpp"
#include "GLDSAAllocator.hpp"
#include "GLDSAUnique.hpp"




namespace josh::dsa {
namespace {

template<typename T>
using UniqueBuffer = GLUnique<RawBuffer<T>>;


[[maybe_unused]]
void buffer_operations() noexcept {

    UniqueBuffer<float> buf;

    buf.allocate_storage(1,
        BufferStorageMode::StaticServer,
        BufferStoragePermittedMapping::ReadWrite);

    auto mapped = buf.map_range_for_write(0, 1,
        BufferMappingWriteAccess::SynchronizedMustFlushExplicitly,
        BufferMappingPreviousContents::InvalidateMappedRange,
        BufferMappingPersistence::NotPersistent);

    mapped[0] = 1.f;

    buf.flush_mapped_range(0, 1);
    buf.unmap_current();
}





} // namespace
// template<>
// struct uniform_traits<glm::vec3> {
//     static void set(RawShaderProgram<> program, Location location, const glm::vec3& v) noexcept {
//         program.set_uniform_vec3v(location, 1, glm::value_ptr(v));
//     }
// };

template<size_t S>
struct uniform_traits<GLint, GLfloat[S]> {
    static void set(RawShaderProgram<> program, Location location, GLsizei count, const GLfloat* data) noexcept {
        program.set_uniform_floatv(location, count, data);
    }
};
namespace {


[[maybe_unused]]
void shadheoadioqu() {
    RawShaderProgram<> p{ 902 };
    // RawVertexShader<> sh{ 99 };
    // p.attach_shader(sh);
    p.uniform(Location{ 0 }, 1);
    p.uniform(Location{ 0 }, 0);
    glm::vec3 v{};
    // p.uniform("color", glm::vec3{ 1.f, 0.f, 1.f });
    // p.uniform("color", v);

}



[[maybe_unused]]
inline void foooo(Layer layer) noexcept {
    static_cast<GLint>(layer);
}


[[maybe_unused]]
void adddsda_remove_later_im_sorry() {
    RawTexture2D<GLMutable> tex{ 32 };
    tex.set_sampler_wrap_all(Wrap::ClampToEdge);
    tex.set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Linear);
    RawTexture2D<GLConst> ct{ tex };
    RawTexture2DMS<GLMutable> tms{ 12 };
    tms.allocate_storage({ 1, 1 }, PixelInternalFormat::RGBA8, NumSamples{ 4 }, SampleLocations::Fixed);
    // tms.allocate_storage({1, 1}, TexSpecMS{{}, {}, {}});
    // tex.allocate_storage({1, 1}, { enum_cast<PixelInternalFormat>(gl::GL_SIGNED_RGB_UNSIGNED_ALPHA_NV) });
    // tex.allocate_storage(Size2I{1, 1}, TexSpec{ PixelInternalFormat::Compressed_SRGBA_BPTC_UNorm });
    tex.allocate_storage({ 1, 1 }, PixelInternalFormat::Compressed_SRGBA_BPTC_UNorm, NumLevels{ 7 });
    pixel::RGBA arr[4];
    tex.sub_image_region({ 0, 0 }, { 2, 2 }, arr);

    RawTexture2DArray<> t2darr{ 732 };
    t2darr.allocate_storage({ 16, 16 }, 32, PixelInternalFormat::RGBA32F, NumLevels{ 5 });
    RawTexture3D<GLMutable> t3d{ 9203 };
    t3d.allocate_storage({ 12, 23, 2 }, PixelInternalFormat::RGBA8, NumLevels{ 7 });
    t3d.invalidate_image_region({ 0, 0, 0 }, { 1, 1, 1 }, 6);
    t3d.sub_image_region({}, { 12, 23, 2 }, arr, MipLevel{ 0 });
    t3d.generate_mipmaps();
    tex.bind_to_readonly_image_unit(ImageUnitFormat::RGBA16, 0, MipLevel{ 0 });
    // tex.bind_layer_to_read_image_unit(0, {}, 0);
    t3d.bind_layer_to_readonly_image_unit(Layer{ 0 }, ImageUnitFormat::RGBA8, 0);

    tex.set_sampler_compare_ref_to_texture(true);
    tex.set_sampler_compare_func(CompareOp::NotEqual);

    // UniqueTexture2D tex{};
    pixel::RGBA pixel_data[16 * 16];
    // tex.allocate_storage({ 512, 512 }, { PixelInternalFormat::Compressed_RGBA_BPTC_UNorm }, 7);
    tex.fill_image(pixel::RGBAF{ 1.f, 0.f, 0.f, 1.f });

    tex.sub_image_region({ 127, 127 }, { 16, 16 }, pixel_data);
    tex.generate_mipmaps();

    tex.set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Nearest);
    tex.set_sampler_wrap_all(Wrap::ClampToEdge);

    tex.bind_to_texture_unit(0);
    RawTexture2D t1{ 1 };
    RawTexture2D<GLMutable> t2{ 2 };
    t2 = t1;


    RawCubemapArray<> c{ 9 };
    c.allocate_storage({ 64, 64 }, 6, PixelInternalFormat::RGBA8, NumLevels{ 5 });

    {
        RawTexture2D t2d{ 2 };
        RawTexture3D t3d{ 3 };
        t2d.copy_image_region_to({}, { 512, 512 }, t3d, { 0, 0, 8 });
        t3d.copy_image_region_to({}, { 64,  64  }, t2d, { 0, 0 });
        RawTextureBuffer<> bu{ 9 };

    }




}



} // namespace
} // namespace josh::dsa {
