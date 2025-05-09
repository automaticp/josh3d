#include "Common.hpp"
#include "GLAPICore.hpp"
#include "GLObjects.hpp"
#include "GLPixelPackTraits.hpp"
#include "PixelPackTraits.hpp"
#include "ContainerUtils.hpp"
#include "CoroCore.hpp"
#include "GLObjectHelpers.hpp"
#include "GLTextures.hpp"
#include "RuntimeError.hpp"
#include "detail/SPNG.hpp"
#include "DefaultImporters.hpp"
#include "AssetImporter.hpp"
#include "Channels.hpp"
#include "Ranges.hpp"
#include "MallocSupport.hpp"
#include "ResourceFiles.hpp"
#include "TextureHelpers.hpp"
#include "ImageData.hpp"
#include <fmt/format.h>
#include <spng.h>


namespace josh {
namespace {


using Format = TextureFile::StorageFormat;


struct EncodedImage {
    unique_malloc_ptr<chan::UByte[]> data;
    Size2I                           resolution;
    size_t                           num_channels;
    size_t                           size_bytes;
    Format                           format;
    auto span() const noexcept -> Span<byte> { return { data.get(), size_bytes }; }
};


[[nodiscard]]
auto encode_texture_async_raw(
    AssetImporterContext&  context [[maybe_unused]],
    ImageData<chan::UByte> image)
        -> Job<EncodedImage>
{
    // Incredible implementation. Not even going to suspend.
    EncodedImage result{
        .data         = nullptr, // Don't release yet, that resets the resolution/channels.
        .resolution   = Size2I(image.resolution()),
        .num_channels = image.num_channels(),
        .size_bytes   = image.size_bytes(),
        .format       = Format::RAW,
    };
    result.data = image.release();
    co_return result;
}


[[nodiscard]]
auto encode_texture_async_png(
    AssetImporterContext&  context,
    ImageData<chan::UByte> image)
        -> Job<EncodedImage>
{
    co_await reschedule_to(context.thread_pool());


    auto      ctx_owner = detail::make_spng_encoding_context();
    spng_ctx* ctx       = ctx_owner.get();
    int       err       = 0;

    err = spng_set_option(ctx, SPNG_ENCODE_TO_BUFFER, 1);
    if (err) throw RuntimeError(fmt::format("SPNG context option error: {}.", spng_strerror(err)));

    const uint8_t color_type = [&]{
        switch (image.num_channels()) {
            case 3: return SPNG_COLOR_TYPE_TRUECOLOR;
            case 4: return SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
            default: std::terminate();
        }
    }();

    spng_ihdr header{
        .width      = uint32_t(image.resolution().width),
        .height     = uint32_t(image.resolution().height),
        .bit_depth  = 8,
        .color_type = color_type,
        .compression_method = {}, // Default.
        .filter_method      = {}, // Default.
        .interlace_method   = {}, // Default.
    };
    err = spng_set_ihdr(ctx, &header);
    if (err) throw RuntimeError(fmt::format("SPNG context header error: {}.", spng_strerror(err)));

    const spng_format format = SPNG_FMT_PNG; // Match format in `header`.

    // TODO: Make configurable [0-9]
    const int compression_level = 9;
    err = spng_set_option(ctx, SPNG_IMG_COMPRESSION_LEVEL, compression_level);
    if (err) throw RuntimeError(fmt::format("Could not set compression level {}.", compression_level));

    err = spng_encode_image(ctx, image.data(), image.size_bytes(), format, SPNG_ENCODE_FINALIZE);
    if (err) throw RuntimeError(fmt::format("Failed encoding PNG: {}.", spng_strerror(err)));


    size_t size_bytes;
    auto   data = unique_malloc_ptr<chan::UByte[]>((chan::UByte*)spng_get_png_buffer(ctx, &size_bytes, &err));
    if (not data or err) throw RuntimeError(fmt::format("Failed retrieving PNG buffer: {}.", spng_strerror(err)));

    co_return EncodedImage{
        .data         = MOVE(data),
        .resolution   = Size2I(image.resolution()),
        .num_channels = image.num_channels(),
        .size_bytes   = size_bytes,
        .format       = Format::PNG,
    };

}


[[nodiscard]]
auto encode_texture_async_bc7(
    AssetImporterContext&  context,
    ImageData<chan::UByte> image)
        -> Job<EncodedImage>
{
    safe_unreachable("BC7 not implemented.");
    // TODO
}

auto pick_mip_internal_format(size_t num_channels)
    -> InternalFormat
{
    switch (num_channels) {
        case 3: return InternalFormat::RGB8;
        case 4: return InternalFormat::RGBA8;
        default: break;
    }
    safe_unreachable();
}

auto pick_mip_data_format(size_t num_channels)
    -> PixelDataFormat
{
    switch (num_channels) {
        case 3: return PixelDataFormat::RGB;
        case 4: return PixelDataFormat::RGBA;
        default: break;
    }
    safe_unreachable();
}

[[nodiscard]]
auto generate_mips(
    AssetImporterContext&            context,
    SmallVector<ImageData<byte>, 1>& mips)
        -> Job<>
{
    const auto resolution0  = Extent2I(mips[0].resolution());
    const auto num_channels = mips[0].num_channels();
    const auto num_mips     = max_num_levels(resolution0);
    const auto iformat      = pick_mip_internal_format(num_channels);
    const auto format       = pick_mip_data_format(num_channels);
    const auto type         = PixelDataType::UByte;
    mips.reserve(num_mips);

    // We use the GPU context to generate the mips. We could also do it ourselves
    // or use a custom shader or whatever. For now the glGenerateMipmap() will do.
    co_await reschedule_to(context.offscreen_context());

    UniqueTexture2D texture;
    texture->allocate_storage(resolution0, iformat, num_mips);
    texture->upload_image_region(
        { {}, resolution0 },
        format,
        type,
        mips[0].data(),
        MipLevel(0)
    );
    texture->generate_mipmaps();

    // NOTE: Suspending on a fence here.
    //
    // Not sure if the barrier is needed, since mipmap generation *can* be considered
    // a rendering operation, although it is never explicitly specified as such.
    //
    // 7.13.2: "TEXTURE_UPDATE_BARRIER_BIT: Writes to a texture via Tex(Sub)Image*, ClearTex*Image,
    // CopyTex*, or CompressedTex*, and reads via GetTexImage after the barrier will not
    // execute until all shader writes initiated prior to the barrier complete."
    glapi::memory_barrier(BarrierMask::TextureUpdateBit);
    if (const auto fence = create_fence(); not is_ready(fence))
        co_await context.completion_context().until_ready_on(context.offscreen_context(), fence);

    // NOTE: This is important to make sure we can read the image data
    // back into the arbitrary aligned buffers. Otherwise some reads
    // could fail with INVALID_OPERATION as the buffer would be too small
    // to fit the alignment padding.
    //
    // See "OpenGL 4.6, ch. 8.4.4.1, eq. (8.2)" for how the alignment
    // affetcs the computed storage buffer extents.
    //
    // See "OpenGL 4.6, ch. 18.2.2, table 18.1" for a list of packing
    // parameters and their defaults.
    //
    // I have no clue why alignment of 4 was chosen as the default,
    // this is the kind of rotten idea that will trip everyone
    // at least once.
    glapi::set_pixel_pack_alignment(1);

    for (const auto mip_id : irange(1, num_mips)) {
        const auto mip_level  = MipLevel(int(mip_id));
        const auto resolution = texture->get_resolution(mip_level);
        // TODO: Could probably do the allocation *off* of the GPU context.
        // But need to precompute the resoluton ahead of time that way.
        // Too lazy right now.
        ImageData<byte> image{ Extent2S(resolution), num_channels };

        texture->download_image_region_into(
            { {}, resolution },
            format,
            type,
            image.span(),
            mip_level
        );
        mips.emplace_back(MOVE(image));
    }
}

} // namespace


auto import_texture(
    AssetImporterContext context,
    Path                 path,
    ImportTextureParams  params)
        -> Job<UUID>
{
    co_await reschedule_to(context.thread_pool());

    using MIPSpec = TextureFile::MIPSpec;

    // TODO: More formats must be supported.
    assert(
        params.storage_format == Format::RAW ||
        params.storage_format == Format::PNG
        && "Only RAW or PNG formats are supported for now. Sad."
    );

    SmallVector<ImageData<byte>, 1> mips;

    // First we load the data with stb. This allows us to load all kinds of formats.
    mips.emplace_back(load_image_data_from_file<chan::UByte>(File(path), 3, 4));
    const size_t num_channels = mips[0].num_channels();

    if (params.generate_mips) {
        co_await generate_mips(context, mips);
        co_await reschedule_to(context.thread_pool());
    }

    // Job per MIP level.
    SmallVector<Job<EncodedImage>, 1> encode_jobs;

    for (auto& mip : mips) {
        // NOTE: Dropping mip data here by moving it out.
        switch (params.storage_format) {
            using enum Format;
            case RAW: encode_jobs.emplace_back(encode_texture_async_raw(context, MOVE(mip))); break;
            case PNG: encode_jobs.emplace_back(encode_texture_async_png(context, MOVE(mip))); break;
            // TODO: Not supported.
            case BC7: encode_jobs.emplace_back(encode_texture_async_bc7(context, MOVE(mip))); break;
            default: assert(false);
        }
    }
    discard(MOVE(mips));

    co_await until_all_ready(encode_jobs);
    co_await reschedule_to(context.thread_pool());


    const auto get_job_result = [](auto& job) { return MOVE(job.get_result()); };

    Vector<EncodedImage> encoded_mips =
        encode_jobs | transform(get_job_result) | ranges::to<Vector>();

    const auto get_mip_spec = [](const EncodedImage& im)
        -> MIPSpec
    {
        return {
            .size_bytes    = uint32_t(im.size_bytes),
            .width_pixels  = uint16_t(im.resolution.width),
            .height_pixels = uint16_t(im.resolution.height),
            .format        = im.format,
        };
    };

    Vector<MIPSpec> mip_specs =
        encoded_mips | transform(get_mip_spec) | ranges::to<Vector>();

    const Path name = path.stem();
    const ResourcePathHint path_hint{
        .directory = "textures",
        .name      = name.c_str(),
        .extension = "jtxtr",
    };


    const TextureFile::Args args{
        .num_channels = uint16_t(num_channels),
        .mip_specs    = mip_specs,
    };

    const size_t       file_size     = TextureFile::required_size(args);
    const ResourceType resource_type = TextureFile::resource_type;

    auto [uuid, mregion] = context.resource_database().generate_resource(resource_type, path_hint, file_size);

    TextureFile file = TextureFile::create_in(MOVE(mregion), uuid, args);

    // NOTE: Iterating in reverse because that's an address increase order for the TextureFile.
    for (auto [mip_id, encoded] : reverse(enumerate(encoded_mips))) {
        const auto dst_bytes = as_bytes(file.mip_bytes(mip_id));
        const auto src_bytes = encoded.span();
        std::ranges::copy(src_bytes, dst_bytes.begin());
    }

    co_return uuid;
}


} // namespace josh
