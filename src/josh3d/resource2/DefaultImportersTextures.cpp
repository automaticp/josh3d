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
    std::terminate();
    // TODO
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

    // First we load the data with stb. This allows us to load all kinds of formats.
    auto image = load_image_data_from_file<chan::UByte>(File(path), 3, 4);
    const size_t num_channels = image.num_channels();

    // Job per MIP level.
    // TODO: Currently, no mipmapping is supported, so only one job :(
    std::vector<Job<EncodedImage>> encode_jobs;

    switch (params.storage_format) {
        using enum Format;
        case RAW: encode_jobs.emplace_back(encode_texture_async_raw(context, MOVE(image))); break;
        case PNG: encode_jobs.emplace_back(encode_texture_async_png(context, MOVE(image))); break;
        // TODO: Not supported.
        case BC7: encode_jobs.emplace_back(encode_texture_async_bc7(context, MOVE(image))); break;
        default: assert(false);
    }

    co_await until_all_ready(encode_jobs);
    co_await reschedule_to(context.thread_pool());


    const auto get_job_result = [](auto& job) { return MOVE(job.get_result()); };

    std::vector<EncodedImage> encoded_mips =
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

    std::vector<MIPSpec> mip_specs =
        encoded_mips | transform(get_mip_spec) | ranges::to<std::vector>();

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


    co_await reschedule_to(context.local_context());
    auto [uuid, mregion] = context.resource_database().generate_resource(resource_type, path_hint, file_size);
    co_await reschedule_to(context.thread_pool());


    TextureFile file = TextureFile::create_in(MOVE(mregion), uuid, args);

    for (auto [mip_id, encoded] : enumerate(encoded_mips)) {
        const auto dst_bytes = file.mip_bytes(mip_id);
        const auto src_bytes = Span<std::byte>((std::byte*)encoded.data.get(), encoded.size_bytes);
        std::ranges::copy(src_bytes, dst_bytes.begin());
    }

    co_return uuid;
}


} // namespace josh
