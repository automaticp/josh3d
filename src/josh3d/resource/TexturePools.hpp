#pragma once
#include "GLPixelPackTraits.hpp"
#include "GLTextures.hpp"
#include "Pixels.hpp"
#include "Shared.hpp"
#include "GLObjects.hpp"
#include "Filesystem.hpp"
#include "ImageData.hpp"
#include "TextureHelpers.hpp"
#include <unordered_map>


namespace josh {


enum class TextureType {
    Default,
    Diffuse,
    Specular,
    Normal
    // Extend later
};



struct TextureDataLoadContext {
    TextureType type         = TextureType::Default;
    size_t      min_channels = 0;
    size_t      max_channels = 4;
};


using TextureDataLoadResult = Shared<const ImageData2<ubyte_t>>;


class TextureDataPool {
public:
    auto load(const File& file, const TextureDataLoadContext& context)
        -> TextureDataLoadResult;
    void clear() { pool_.clear(); }

private:
    using pool_type = std::unordered_map<File, TextureDataLoadResult>;
    pool_type pool_;

    auto load_from_file(const File& file, const TextureDataLoadContext& context)
        -> TextureDataLoadResult;
};




inline auto TextureDataPool::load(
    const File&                   file,
    const TextureDataLoadContext& context)
        -> TextureDataLoadResult
{
    auto it = pool_.find(file);
    if (it != pool_.end()) {
        return it->second;
    } else {
        auto [emplaced_it, was_emplaced] = pool_.emplace(file, load_from_file(file, context));
        return emplaced_it->second;
    }
}


inline auto TextureDataPool::load_from_file(
    const File&                   file,
    const TextureDataLoadContext& context [[maybe_unused]])
        -> TextureDataLoadResult
{
    return make_shared<const ImageData2<ubyte_t>>(
        load_image_data_from_file<ubyte_t>(file, context.min_channels, context.max_channels)
    );
}





namespace globals {
inline TextureDataPool texture_data_pool;
} // namespace globals








struct TextureHandleLoadContext {
    TextureType type{ TextureType::Default };
};


using TextureHandleLoadResult = SharedTexture2D;


class TextureHandlePool {
public:
    TextureHandlePool(TextureDataPool& upstream) noexcept : upstream_{ &upstream } {}

    auto load(const File& file, const TextureHandleLoadContext& context)
        -> TextureHandleLoadResult;
    void clear() { pool_.clear(); }

private:
    using pool_type     = std::unordered_map<File, TextureHandleLoadResult>;
    using upstream_type = TextureDataPool;
    pool_type      pool_;
    upstream_type* upstream_;

    auto load_from_file(const File& file, const TextureHandleLoadContext& context)
        -> TextureHandleLoadResult;
};




inline auto TextureHandlePool::load(
    const File&                     file,
    const TextureHandleLoadContext& context)
        -> TextureHandleLoadResult
{
    auto it = pool_.find(file);
    if (it != pool_.end()) {
        return it->second;
    } else {
        auto [emplaced_it, was_emplaced] = pool_.emplace(file, load_from_file(file, context));
        return emplaced_it->second;
    }
}


inline auto TextureHandlePool::load_from_file(
    const File&                     file,
    const TextureHandleLoadContext& context)
        -> TextureHandleLoadResult
{
    TextureDataLoadContext upstream_context{ .type = context.type };
    switch (context.type) {
        using enum TextureType;
        case Diffuse: {
            upstream_context.min_channels = 3;
            upstream_context.max_channels = 4;
        } break;
        case Specular: {
            upstream_context.min_channels = 1;
            upstream_context.max_channels = 1;
        } break;
        case Normal: {
            upstream_context.min_channels = 3;
            upstream_context.max_channels = 3;
        } break;
        default: break;
    }

    Shared<const ImageData2<ubyte_t>> data = upstream_->load(file, upstream_context);

    const size_t num_channels = data->num_channels();



    const PixelDataFormat format = [&] {
        switch (num_channels) {
            case 1: return PixelDataFormat::Red;
            case 2: return PixelDataFormat::RG;
            case 3: return PixelDataFormat::RGB;
            case 4: return PixelDataFormat::RGBA;
            default: assert(false);
        }
    }();

    const PixelDataType type = PixelDataType::UByte;

    const InternalFormat iformat = [&] {
        switch (context.type) {
            using enum TextureType;
            case Diffuse: {
                if      (num_channels == 3) { return InternalFormat::SRGB8;  }
                else if (num_channels == 4) { return InternalFormat::SRGBA8; }
                else { assert(false); }
            } break;
            case Specular: {
                if      (num_channels == 1) { return InternalFormat::R8; }
                else { assert(false); }
            } break;
            case Normal: {
                if      (num_channels == 3) { return InternalFormat::RGB8; }
                else { assert(false); }
            } break;
            default: {
                return InternalFormat::RGBA8;
            } break;
        }
    }();

    SharedTexture2D new_handle = create_material_texture_from_image_data(*data, format, type, iformat);

    return new_handle;
}




namespace globals {
inline TextureHandlePool texture_handle_pool{ texture_data_pool };
} // namespace globals



} // namespace josh
