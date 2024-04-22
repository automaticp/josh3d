#include "Primitives.hpp"
#include "AssetManager.hpp"
#include "Filesystem.hpp"
#include "Future.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "PixelPackTraits.hpp"
#include "GLPixelPackTraits.hpp"
#include "PixelData.hpp"
#include "Pixels.hpp"
#include "TextureHelpers.hpp"
#include "VertexPNUTB.hpp"


namespace josh {


namespace {


template<typename PixelT>
auto make_single_pixel_image_data(PixelT pixel)
    -> PixelData<PixelT>
{
    PixelData<PixelT> image{ { 1, 1 } };
    image.at({ 0, 0 }) = pixel;
    return image;
}


template<typename PixelT>
auto create_single_pixel_texture(
    PixelT pixel,
    InternalFormat iformat)
        -> UniqueTexture2D
{
    auto tex_data = make_single_pixel_image_data(pixel);
    UniqueTexture2D texture = create_material_texture_from_pixel_data(tex_data, iformat);
    return texture;
}


UniqueCubemap load_debug_skybox() {

    CubemapPixelData data =
        load_cubemap_from_json<pixel::RGB>(File("data/skyboxes/debug/skybox.json"));

    UniqueCubemap cube =
        create_skybox_from_cubemap_data(data, InternalFormat::SRGB8);

    cube->set_sampler_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
    return cube;
}


Mesh load_simple_mesh(AssetManager& assman, Path path) {
    auto shared_mesh = get_result(assman.load_model(AssetPath{ std::move(path), {} })).meshes.at(0);
    make_available<Binding::ArrayBuffer>       (shared_mesh.vertices->id());
    make_available<Binding::ElementArrayBuffer>(shared_mesh.indices->id() );
    return Mesh::from_buffers<VertexPNUTB>(std::move(shared_mesh.vertices), std::move(shared_mesh.indices));
}


} // namespace



Primitives::Primitives(AssetManager& assman)
    : default_diffuse_texture_{
        create_single_pixel_texture(pixel::RGB{ 0xB0, 0xB0, 0xB0 }, InternalFormat::SRGB8)
    }
    , default_specular_texture_{
        create_single_pixel_texture(pixel::Red{ 0x00 }, InternalFormat::R8)
    }
    , default_normal_texture_{
        create_single_pixel_texture(pixel::RGB{ 0x7F, 0x7F, 0xFF }, InternalFormat::RGB8)
    }
    , debug_skybox_cubemap_{
        load_debug_skybox()
    }
    , plane_mesh_{
        load_simple_mesh(assman, "data/primitives/plane.obj")
    }
    , box_mesh_{
        load_simple_mesh(assman, "data/primitives/box.obj")
    }
    , sphere_mesh_{
        load_simple_mesh(assman, "data/primitives/sphere.obj")
    }
    , quad_mesh_{
        load_simple_mesh(assman, "data/primitives/quad.obj")
    }
{}






} // namespace josh
