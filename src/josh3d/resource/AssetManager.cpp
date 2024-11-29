#include "AssetManager.hpp"
#include "Asset.hpp"
#include "Channels.hpp"
#include "ContainerUtils.hpp"
#include "CubemapData.hpp"
#include "Filesystem.hpp"
#include "GLAPICore.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "GLPixelPackTraits.hpp"
#include "GLTextures.hpp"
#include "ImageData.hpp"
#include "MeshRegistry.hpp"
#include "MeshData.hpp"
#include "OffscreenContext.hpp"
#include "TextureHelpers.hpp"
#include "CategoryCasts.hpp"
#include "ThreadPool.hpp"
#include "VertexPNUTB.hpp"
#include "Coroutines.hpp"
#include <assimp/BaseImporter.h>
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <boost/scope/defer.hpp>
#include <boost/scope/scope_fail.hpp>
#include <exception>
#include <range/v3/view/enumerate.hpp>
#include <cassert>
#include <cstdint>
#include <optional>
#include <range/v3/view/transform.hpp>
#include <utility>


namespace josh {


AssetManager::AssetManager(
    ThreadPool&        loading_pool,
    OffscreenContext&  offscreen_context,
    CompletionContext& completion_context,
    MeshRegistry&      mesh_registry
)
    : thread_pool_       { loading_pool       }
    , offscreen_context_ { offscreen_context  }
    , completion_context_{ completion_context }
    , mesh_registry_     { mesh_registry      }
{}




void AssetManager::update() {}



namespace {


auto pick_pixel_data_format(size_t num_channels)
    -> PixelDataFormat
{
    switch (num_channels) {
        case 1: return PixelDataFormat::Red;
        case 2: return PixelDataFormat::RG;
        case 3: return PixelDataFormat::RGB;
        case 4: return PixelDataFormat::RGBA;
        default: assert(false); return {};
    }
}


auto pick_internal_format(size_t num_channels, ImageIntent intent)
    -> InternalFormat
{
    // TODO: Why are we failing when the num_channels is "incorrect"?
    // What would happen if we just used the closest available format?
    switch (intent) {
        case ImageIntent::Albedo:
            switch (num_channels) {
                case 3: return InternalFormat::SRGB8;
                case 4: return InternalFormat::SRGBA8;
                default: assert(false); return {};
            }
        case ImageIntent::Specular:
            switch (num_channels) {
                case 1: return InternalFormat::R8;
                default: assert(false); return {};
            }
        case ImageIntent::Normal:
            switch (num_channels) {
                case 3: return InternalFormat::RGB8;
                default: assert(false); return {};
            }
        case ImageIntent::Alpha:
            switch (num_channels) {
                case 1: return InternalFormat::R8;
                default: assert(false); return {};
            }
        case ImageIntent::Heightmap:
            switch (num_channels) {
                case 1: return InternalFormat::R8;
                default: assert(false); return {};
            }
        case ImageIntent::Unknown:
            switch (num_channels) {
                case 1: return InternalFormat::R8;
                case 2: return InternalFormat::RG8;
                case 3: return InternalFormat::RGB8;
                case 4: return InternalFormat::RGBA8;
                default: assert(false); return {};
            }
    }
    // Don't return here, clang-tidy will warn if return is missing above.
    std::terminate();
}


auto pick_internal_format(size_t num_channels, CubemapIntent intent)
    -> InternalFormat
{
    switch (intent) {
        case CubemapIntent::Skybox:
            switch (num_channels) {
                case 3: return InternalFormat::SRGB8;
                case 4: return InternalFormat::SRGBA8;
                default: assert(false); return {};
            }
        case CubemapIntent::Unknown:
            switch (num_channels) {
                case 1: return InternalFormat::R8;
                case 2: return InternalFormat::RG8;
                case 3: return InternalFormat::RGB8;
                case 4: return InternalFormat::RGBA8;
                default: assert(false); return {};
            }
    }
    std::terminate();
}


} // namespace




auto AssetManager::load_texture(AssetPath path, ImageIntent intent)
    -> Job<SharedTextureAsset>
{
    // This holds the AssetManager alive until all the coroutines have finished/terminated.
    const auto task_guard = task_counter_.obtain_task_guard();


    // Scheduled on the thread pool.
    {
        co_await reschedule_to(thread_pool_);
    }


    // Check the cache first.
    // TODO: This should probably consider ImageInternt as part of the key.
    if (std::optional<SharedTextureAsset> asset =
        co_await cache_.get_if_cached_or_join_pending<AssetKind::Texture>(path))
    {
        // We either got a value from cache, or we suspended as pending and another job resolved it later.
        co_return move_out(asset);
    }
    // Otherwise we need to load and resolve it ourselves.

    const auto on_exception = // Resolve the pending requests with the same exception.
        boost::scope::scope_fail([&] { cache_.fail_and_resolve_pending<AssetKind::Texture>(path); });


    // Do the image loading/decompression with stb.
    auto [min_channels, max_channels] = image_intent_minmax_channels(intent);

    ImageData<chan::UByte> data =
        load_image_data_from_file<chan::UByte>(File(path.entry()), min_channels, max_channels);

    const size_t          num_channels = data.num_channels();
    const PixelDataType   type         = PixelDataType::UByte;
    const PixelDataFormat format       = pick_pixel_data_format(num_channels);
    const InternalFormat  iformat      = pick_internal_format(num_channels, intent);


    // Reschedule to the offscreen gl context.
    {
        co_await reschedule_to(offscreen_context_);
    }


    // Upload image from the offscreen context.
    SharedTexture2D texture = create_material_texture_from_image_data(data, format, type, iformat);

    glapi::flush();      // Flush the texture upload (hopefully).
    discard(MOVE(data)); // In the meantime, don't need the data anymore, destroy it.
    glapi::finish();     // Wait until commands complete.
    // TODO: Could await on a FenceSync instead, but it's a bother to implement.


    // Resolve from the offscreen context.
    StoredTextureAsset asset{
        .path    = MOVE(path),
        .intent  = intent,
        .texture = MOVE(texture),
    };

    // TODO: Might be worth resolving pending from a different executor,
    // since the offscreen context is a single thread and can get pretty busy.
    cache_.cache_and_resolve_pending(asset.path, asset);
    co_return MOVE(asset);
}




namespace {


auto load_image_data(File file, ImageIntent intent, ThreadPool& thread_pool)
    -> Job<ImageData<chan::UByte>>
{
    {
        co_await reschedule_to(thread_pool);
    }

    auto [min_channels, max_channels] = image_intent_minmax_channels(intent);

    co_return load_image_data_from_file<chan::UByte>(file, min_channels, max_channels);
}


} // namespace




auto AssetManager::load_cubemap(AssetPath path, CubemapIntent intent)
    -> Job<SharedCubemapAsset>
{
    using std::views::transform;
    const auto task_guard = task_counter_.obtain_task_guard();


    {
        co_await reschedule_to(thread_pool_);
    }


    if (std::optional<SharedCubemapAsset> asset =
        co_await cache_.get_if_cached_or_join_pending<AssetKind::Cubemap>(path))
    {
        co_return move_out(asset);
    }

    const auto on_exception =
        boost::scope::scope_fail([&]{ cache_.fail_and_resolve_pending<AssetKind::Cubemap>(path); });


    std::array<File, 6> files = parse_cubemap_json_for_files(File(path.entry()));

    auto submit_side_loading = [&](const File& file)
        -> Job<ImageData<chan::UByte>>
    {
        return load_image_data(file, ImageIntent::Albedo, thread_pool_);
    };

    std::array<Job<ImageData<chan::UByte>>, 6> jobs{
        submit_side_loading(files[0]),
        submit_side_loading(files[1]),
        submit_side_loading(files[2]),
        submit_side_loading(files[3]),
        submit_side_loading(files[4]),
        submit_side_loading(files[5]),
    };


    {
        co_await completion_context_.until_all_ready(jobs);
        co_await reschedule_to(offscreen_context_);
    }


    auto extract_data_result = [&](size_t side) {
        assert(jobs[side].is_ready());
        return MOVE(jobs[side]).get_result();
    };

    CubemapImageData<chan::UByte> data{
        extract_data_result(0),
        extract_data_result(1),
        extract_data_result(2),
        extract_data_result(3),
        extract_data_result(4),
        extract_data_result(5),
    };

    discard(MOVE(jobs));

    const size_t          num_channels = data.sides().at(0).num_channels();
    const PixelDataType   type         = PixelDataType::UByte;
    const PixelDataFormat format       = pick_pixel_data_format(num_channels);
    const InternalFormat  iformat      = pick_internal_format(num_channels, intent);

    SharedCubemap cubemap = [&]{
        switch (intent) {
            case CubemapIntent::Skybox:
                return create_skybox_from_cubemap_image_data(data, format, type, iformat);
            case CubemapIntent::Unknown:
                return create_material_cubemap_from_image_data(data, format, type, iformat);
        }
        std::terminate();
    }();

    glapi::flush();
    discard(MOVE(data));
    glapi::finish();


    StoredCubemapAsset asset{
        .path    = MOVE(path),
        .intent  = intent,
        .cubemap = MOVE(cubemap),
    };

    cache_.cache_and_resolve_pending(asset.path, asset);
    co_return MOVE(asset);
}




namespace {


auto get_path_to_ai_texture(
    const Path&       parent_path,
    const aiMaterial& material,
    aiTextureType     type)
        -> AssetPath
{
    aiString filename;
    material.GetTexture(type, 0, &filename);
    return { parent_path / filename.C_Str() };
};


auto get_ai_texture_type(const AssetPath& path, ImageIntent intent) -> aiTextureType {
    switch (intent) {
        case ImageIntent::Albedo:
            return aiTextureType_DIFFUSE;
        case ImageIntent::Specular:
            return aiTextureType_SPECULAR;
        case ImageIntent::Normal:
            if (Assimp::BaseImporter::SimpleExtensionCheck(path.entry(), "obj")) {
                return aiTextureType_HEIGHT;
            } else {
                return aiTextureType_NORMALS;
            }
        case ImageIntent::Alpha:
            return aiTextureType_OPACITY;
        case ImageIntent::Heightmap:
            return aiTextureType_DISPLACEMENT;
        case ImageIntent::Unknown:
        default:
            // ???
            return aiTextureType_UNKNOWN;
    }
};




auto v2v(const aiVector3D& v) noexcept
    -> glm::vec3
{
    return { v.x, v.y, v.z };
}




auto get_mesh_data(const aiMesh& mesh)
    -> MeshData<VertexPNUTB>
{
    auto verts      = std::span(mesh.mVertices,         mesh.mNumVertices);
    auto uvs        = std::span(mesh.mTextureCoords[0], mesh.mNumVertices);
    auto normals    = std::span(mesh.mNormals,          mesh.mNumVertices);
    auto tangents   = std::span(mesh.mTangents,         mesh.mNumVertices);
    auto bitangents = std::span(mesh.mBitangents,       mesh.mNumVertices);

    if (!normals.data())    { throw error::AssetContentsParsingError("Mesh data does not contain Normals");             }
    if (!uvs.data())        { throw error::AssetContentsParsingError("Mesh data does not contain Texture Coordinates"); }
    if (!tangents.data())   { throw error::AssetContentsParsingError("Mesh data does not contain Tangents");            }
    if (!bitangents.data()) { throw error::AssetContentsParsingError("Mesh data does not contain Bitangents");          }

    std::vector<VertexPNUTB> vertex_data;
    vertex_data.reserve(verts.size());

    for (size_t i{ 0 }; i < verts.size(); ++i) {
        const VertexPNUTB vert{
            .position  = v2v(verts     [i]),
            .normal    = v2v(normals   [i]),
            .uv        = v2v(uvs       [i]),
            .tangent   = v2v(tangents  [i]),
            .bitangent = v2v(bitangents[i]),
        };
        vertex_data.emplace_back(vert);
    }

    auto faces = std::span(mesh.mFaces, mesh.mNumFaces);

    std::vector<unsigned int> indices;
    indices.reserve(faces.size() * 3);

    for (const auto& face : faces) {
        for (const auto& index : std::span(face.mIndices, face.mNumIndices)) {
            indices.emplace_back(index);
        }
    }

    return { MOVE(vertex_data), MOVE(indices) };
}


} // namespace




auto AssetManager::load_model(AssetPath path)
    -> Job<SharedModelAsset>
{
    // NOTE: See load_texture() for comments on the general flow of execution.
    using ranges::views::enumerate;
    const auto task_guard = task_counter_.obtain_task_guard();


    {
        co_await reschedule_to(thread_pool_);
    }


    if (std::optional<SharedModelAsset> asset =
        co_await cache_.get_if_cached_or_join_pending<AssetKind::Model>(path))
    {
        co_return move_out(asset);
    }

    const auto on_exception =
        boost::scope::scope_fail([&] { cache_.fail_and_resolve_pending<AssetKind::Model>(path); });



    using TextureIndex    = int32_t;
    using MaterialIndex   = size_t;
    using TextureJobIndex = size_t;

    struct TextureInfo {
        TextureIndex id     = -1;
        ImageIntent  intent = ImageIntent::Unknown;
    };

    struct MaterialRefs {
        TextureIndex diffuse_id  = -1;
        TextureIndex specular_id = -1;
        TextureIndex normal_id   = -1;
    };

    struct MeshInfo {
        AssetPath             path;
        MeshData<VertexPNUTB> data;
        LocalAABB             aabb;
        MaterialRefs          material;
        MaterialIndex         material_id;
    };


    // TODO: This is a bit of a waste, but thread_locals are no-go across threads.
    // TODO: Maybe at least use a monotonic buffer for all of these allocations?
    std::vector<MaterialRefs>                  material_refs;  // Order: Materials.
    std::vector<MeshInfo>                      mesh_infos;     // Order: Meshes.

    // Need this to get the Job from TextureIndex, since Jobs are unordered.
    std::vector<TextureJobIndex>               texid2jobid;    // Order: Textures.
    std::unordered_map<AssetPath, TextureInfo> path2texinfo;   // Order: Texture Jobs.
    std::vector<SharedJob<SharedTextureAsset>> texture_jobs;   // Order: Texture Jobs.
    std::vector<SharedTextureAsset>            texture_assets; // Order: Texture Jobs.

    // This is the primary result of this job.
    std::vector<SharedMeshAsset>               mesh_assets;    // Order: Meshes.

    // NOTE: reserve()/resize() are done as-needed, if we get that far.


    auto get_texture_asset_by_id = [&](TextureIndex id)
        -> std::optional<SharedTextureAsset>
    {
        if (id >= 0) {
            return texture_assets[texid2jobid[id]];
        } else {
            return std::nullopt;
        }
    };


    // Will be used to assign new indices for textures. These are global for all textures in all materials.
    TextureIndex next_texture_index{ 0 };

    auto assign_texture_index = [&](const aiMaterial& ai_material, ImageIntent intent)
        -> TextureIndex
    {
        const aiTextureType ai_type = get_ai_texture_type(path, intent);
        const bool          exists  = ai_material.GetTextureCount(ai_type);

        if (!exists) { return -1; } // If no texture corresponding to this ImageIntent in the material.

        AssetPath texture_path = get_path_to_ai_texture(path.entry().parent_path(), ai_material, ai_type);

        const TextureInfo texture_info{
            .id     = next_texture_index,
            .intent = intent,
        };

        auto [it, was_emplaced] = path2texinfo.emplace(MOVE(texture_path), texture_info);

        if (was_emplaced) { ++next_texture_index; }

        // If it wasn't emplaced, then it was already there,
        // Either way, we can get the index from `it`.
        return it->second.id;
    };




    // NOTE: The importer *can* be made thread local, since the data is only really needed
    // until the first suspension point. For that, we need to scope *it* and other scene-related variables.
    {
        thread_local Assimp::Importer importer;
        BOOST_SCOPE_DEFER [&] { importer.FreeScene(); };

        constexpr auto flags =
            aiProcess_Triangulate              |
            aiProcess_GenUVCoords              | // Uhh, how?
            aiProcess_GenSmoothNormals         |
            aiProcess_CalcTangentSpace         |
            aiProcess_GenBoundingBoxes         |
            aiProcess_ImproveCacheLocality     |
            aiProcess_OptimizeGraph            |
        //  aiProcess_OptimizeMeshes           |
            aiProcess_RemoveRedundantMaterials;

        const aiScene* scene = importer.ReadFile(path.entry(), flags);

        if (!scene) { throw error::AssetFileImportFailure(path.entry(), importer.GetErrorString()); }

        const size_t num_meshes    = scene->mNumMeshes;
        const size_t num_materials = scene->mNumMaterials;

        const auto ai_meshes    = std::span(scene->mMeshes,    scene->mNumMeshes   );
        const auto ai_materials = std::span(scene->mMaterials, scene->mNumMaterials);


        material_refs.reserve(num_materials);


        // Just do a prepass where we resolve a unique set of paths to load,
        // and set the indices for each texture.
        for (const auto* ai_material : ai_materials) {
            material_refs.emplace_back(MaterialRefs{
                .diffuse_id  = assign_texture_index(*ai_material, ImageIntent::Albedo),
                .specular_id = assign_texture_index(*ai_material, ImageIntent::Specular),
                .normal_id   = assign_texture_index(*ai_material, ImageIntent::Normal),
            });
        }


        // Now we have a set of texture paths that we need to load.
        // We'll submit jobs for them and then do mesh loading in parallel.
        const size_t num_textures = path2texinfo.size();

        texid2jobid .resize (num_textures);
        texture_jobs.reserve(num_textures);

        for (const auto& item : path2texinfo) {
            const auto& [path, tex_info] = item;
            texid2jobid[tex_info.id] = texture_jobs.size();
            texture_jobs.emplace_back(load_texture(path, tex_info.intent));
        }


        // Submitted load requests for all the textures we need, now extract mesh data on this thread.
        // We'll probably just upload all meshes at once, not convert->upload one-by-one, as that
        // keeps the offscreen context less busy.


        mesh_infos.reserve(num_meshes);

        for (const auto* ai_mesh : ai_meshes) {
            auto apath = AssetPath{ path.entry(), ai_mesh->mName.C_Str() };
            auto data  = get_mesh_data(*ai_mesh);
            auto aabb  = LocalAABB{ v2v(ai_mesh->mAABB.mMin), v2v(ai_mesh->mAABB.mMax) };

            const auto  material_id  = size_t(ai_mesh->mMaterialIndex);
            const auto& material_ref = material_refs[material_id];

            mesh_infos.push_back({
                .path        = MOVE(apath),
                .data        = MOVE(data),
                .aabb        = aabb,
                .material    = material_ref,
                .material_id = material_id,
            });
        }


        mesh_assets.reserve(num_meshes);
    }


    // Now we go to offscreen to upload the meshes.
    // This better be quick, as this context is only one thread.
    {
        co_await reschedule_to(offscreen_context_);
    }


    for (auto& mesh_info : mesh_infos) {

        const std::span<const VertexPNUTB> verts   = mesh_info.data.vertices;
        const std::span<const GLuint>      indices = mesh_info.data.elements;

        SharedBuffer<VertexPNUTB> verts_buf   = specify_buffer(verts,   { .mode = StorageMode::StaticServer });
        SharedBuffer<GLuint>      indices_buf = specify_buffer(indices, { .mode = StorageMode::StaticServer });

        StoredMeshAsset mesh_asset{
            .path     = MOVE(mesh_info.path),
            .aabb     = mesh_info.aabb,
            .vertices = MOVE(verts_buf),
            .indices  = MOVE(indices_buf),
            .diffuse  = {}, // NOTE: Needs to be set later.
            .specular = {}, // NOTE: Needs to be set later.
            .normal   = {}, // NOTE: Needs to be set later.
        };

        mesh_assets.emplace_back(MOVE(mesh_asset));
        discard(MOVE(mesh_info.data)); // TODO: Is this really that necessary? Maybe use monotonic buffer?
    }


    // Then, we need to wait on all of the textures.
    //
    // - We can switch back to the thread pool and just block.
    //   Not only does this take the thread pool resources away, which is sad,
    //   but this also can deadlock the thread pool under certain conditions.
    //
    // - Or we can submit this to some kind of WhenAll handler,
    //   that sweeps through requests like these.
    //   There, we *just somehow wait* until all of the subtasks
    //   are complete, and *only then* reschedule back to the thread pool.


    {
        co_await completion_context_.until_all_ready(texture_jobs);
        co_await reschedule_to(thread_pool_);
    }


    const size_t num_textures = texture_jobs.size();
    texture_assets.reserve(num_textures);
    for (auto& texture_job : texture_jobs) {
        assert(texture_job.is_ready());
        texture_assets.emplace_back(texture_job.get_result());
    }
    texture_jobs.clear();


    // Finally, we pass over all of the meshes and resolve the textures.
    for (auto [i, mesh_asset] : enumerate(mesh_assets)) {
        const auto& refs = mesh_infos[i].material;
        mesh_asset.diffuse  = get_texture_asset_by_id(refs.diffuse_id );
        mesh_asset.specular = get_texture_asset_by_id(refs.specular_id);
        mesh_asset.normal   = get_texture_asset_by_id(refs.normal_id  );
    }


    // Done, whew...
    StoredModelAsset asset{
        .path   = MOVE(path),
        .meshes = MOVE(mesh_assets),
    };

    cache_.cache_and_resolve_pending(asset.path, asset);
    co_return MOVE(asset);
}


} // namespace josh
