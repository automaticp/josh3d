#include "AssetManager.hpp"
#include "Channels.hpp"
#include "Future.hpp"
#include "GLAPICore.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "GLPixelPackTraits.hpp"
#include "GLTextures.hpp"
#include "TextureHelpers.hpp"
#include "VertexPNUTB.hpp"
#include <glfwpp/window.h>
#include <assimp/Importer.hpp>
#include <assimp/BaseImporter.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/vector3.h>
#include <exception>
#include <filesystem>
#include <mutex>
#include <stop_token>
#include <optional>
#include <type_traits>
#include <ranges>




namespace josh {


AssetManager::AssetManager(
    VirtualFilesystem&  vfs,
    const glfw::Window& shared_context)
    : vfs_{ vfs }
    , dispatch_thread_ { [this](std::stop_token stoken) { dispatch_thread_loop (std::move(stoken)); } }
    , loading_thread_  { [this](std::stop_token stoken) { loading_thread_loop  (std::move(stoken)); } }
    , uploading_thread_{
        [&, this](std::stop_token stoken) {
            glfw::WindowHints{
                .visible             = false,
                .contextVersionMajor = 4,
                .contextVersionMinor = 6,
                .openglProfile       = glfw::OpenGlProfile::Core,
            }.apply();

            glfw::Window window{ 1, 1, "Uploading Context", nullptr, &shared_context };
            glfw::makeContextCurrent(window);

            uploading_thread_loop(std::move(stoken));
        }
    }
{}




auto AssetManager::load_model(const AssetVPath& vpath)
    -> Future<SharedModelAsset>
{
    auto [future, promise] = make_future_promise_pair<SharedModelAsset>();
    try {
        auto file = vfs_.resolve_file(vpath.file);
        AssetPath apath{ std::filesystem::canonical(file.path()), vpath.subpath };
        dispatch_requests_.emplace(std::move(apath), std::move(promise));
    } catch (...) {
        set_exception(std::move(promise), std::current_exception());
    }
    return std::move(future);
}


auto AssetManager::load_model(const AssetPath& path)
    -> Future<SharedModelAsset>
{
    auto [future, promise] = make_future_promise_pair<SharedModelAsset>();
    try {
        AssetPath apath{ std::filesystem::canonical(path.file), path.subpath };
        dispatch_requests_.emplace(std::move(apath), std::move(promise));
    } catch (...) {
        set_exception(std::move(promise), std::current_exception());
    }
    return std::move(future);
}





void AssetManager::dispatch_thread_loop(std::stop_token stoken) { // NOLINT
    while (!stoken.stop_requested()) {
        std::optional<DispatchRequest> request = dispatch_requests_.wait_and_pop(stoken);
        if (!request.has_value()) {
            break;
        }
        handle_dispatch_request(std::move(request.value()));;
    }
}


void AssetManager::loading_thread_loop(std::stop_token stoken) { // NOLINT
    while (!stoken.stop_requested()) {
        std::optional<LoadRequest> request = load_requests_.wait_and_pop(stoken);
        if (!request.has_value()) {
            break;
        }
        handle_load_request(std::move(request.value()));
    }
}


void AssetManager::uploading_thread_loop(std::stop_token stoken) { // NOLINT
    while (!stoken.stop_requested()) {
        std::optional<UploadRequest> request = upload_requests_.wait_and_pop(stoken);
        if (!request.has_value()) {
            break;
        }
        handle_upload_request(std::move(request.value()));
    }
}



auto AssetManager::stored_to_shared(const StoredTextureAsset& stored)
    -> SharedTextureAsset
{
    return SharedTextureAsset{
        .path    = stored.path,
        .intent  = stored.intent,
        .texture = stored.texture
    };
}


auto AssetManager::stored_to_shared(const StoredMeshAsset& stored)
    -> SharedMeshAsset
{
    auto transform_tex = [](const std::optional<StoredTextureAsset>& opt)
        -> std::optional<SharedTextureAsset>
    {
        if (opt.has_value()) {
            return stored_to_shared(*opt);
        } else {
            return std::nullopt;
        }
    };

    return SharedMeshAsset{
        .path     = stored.path,
        .aabb     = stored.aabb,
        .vertices = stored.vertices,
        .indices  = stored.indices,
        .diffuse  = transform_tex(stored.diffuse),
        .specular = transform_tex(stored.specular),
        .normal   = transform_tex(stored.normal),
    };
}



auto AssetManager::stored_to_shared(const StoredModelAsset& stored)
    -> SharedModelAsset
{
    auto result_view = stored.meshes
        | std::views::transform([](const StoredMeshAsset& mesh) { return stored_to_shared(mesh); });

    return SharedModelAsset{
        .path   = stored.path,
        .meshes = { result_view.begin(), result_view.end() }
    };
}





void AssetManager::handle_dispatch_request(DispatchRequest&& request) {

    // Check if the Model is in cache, and if so, just resolve it here.
    {
        std::scoped_lock model_cache_lock{ model_cache_mutex_ };
        auto it = model_cache_.find(request.path);
        if (it != model_cache_.end()) {
            set_result(std::move(request.promise), stored_to_shared(it->second));
            return;
        }

        // If we were to release the model_cache_lock here, then the Uploading Thread could
        // both Cache and Resolve Pending requests in-between releasing this lock and acquiring
        // the pending_requests_lock later, which would make the following request go through, redundantly.
        //
        // That request will be rejected very late and will cause useless work, but it's more of an edge case.
        // Still, we don't want that. So keep the cache mutex locked here.

        // Check if the load of this asset is already pending completion by a prevoius request.
        {
            std::scoped_lock pending_requests_lock{ pending_requests_mutex_ };
            auto it = pending_requests_.find(request.path);
            if (it != pending_requests_.end()) {
                // If there's a request already being processed, then just push back
                // this request on top of the current one and return.
                // This will be later picked up by the Uploading Thread and resolved
                // along with the primary request.
                it->second.emplace_back(std::move(request.promise));
                return;
            }
            // If not in pending_requests_, then it's not currently being loaded.
            // Emplace an empty vector to signal that it is.
            pending_requests_.emplace(request.path, decltype(pending_requests_)::mapped_type{});
        } // ~pending_requests_lock
    } // ~model_cache_lock


    load_requests_.emplace(
        LoadRequest{
            .model   = UnresolvedModel{ .path = std::move(request.path) },
            .promise = std::move(request.promise),
        }
    );
}




void AssetManager::handle_load_request(LoadRequest&& request) {

    thread_local Assimp::Importer importer;

    constexpr auto flags =
    // You can EmbedTextures here but I need to move the data out.
        aiProcess_Triangulate              |
        aiProcess_GenUVCoords              | // Uhh, how?
        aiProcess_GenSmoothNormals         |
        aiProcess_CalcTangentSpace         |
        aiProcess_GenBoundingBoxes         |
        aiProcess_ImproveCacheLocality     |
        aiProcess_OptimizeGraph            |
        // aiProcess_OptimizeMeshes           |
        aiProcess_RemoveRedundantMaterials;


    const aiScene* scene = importer.ReadFile(request.model.path.file.c_str(), flags);

    if (!scene) {
        set_exception(
            std::move(request.promise),
            std::make_exception_ptr(
                error::AssetFileImportFailure(std::move(request.model.path.file), importer.GetErrorString())
            )
        );
        return;
    }


    auto v2v  = [](const aiVector3D& v) -> glm::vec3 { return { v.x, v.y, v.z }; };


    // Starting from this step, we are building an `UnresolvedModel`, which is
    // a collection of `UnresolvedMesh`es and `UnresolvedTexture`s
    //
    // For Meshes this step loads the mesh data from disk and produces a `MeshDataAsset`.
    // For Textures this step either loads the images from disk and produces `ImageDataAsset`
    // or it uses a cached `StoredTextureAsset` directly.

    // We begin with Textures.
    try {
        // These thread_locals are reused on each new load,
        // their contents never leave the scope of this function.
        thread_local std::unordered_map<AssetPath, TextureIndex> tex_index_map;
        tex_index_map.clear();

        struct MaterialInfo {
            TextureIndex diffuse_id  = -1;
            TextureIndex specular_id = -1;
            TextureIndex normal_id   = -1;
        };

        thread_local std::vector<MaterialInfo> material_info; // Heap allocation? More like keep allocation.
        material_info.clear();


        // This will be moved to `UnresolvedModel` later.
        std::vector<UnresolvedTexture> textures;



        // Build a map of AssetPath -> TextureIndex AND a vector of Textures at the same time.
        // Each Texture not present in the map gets a new index and a place in the vector.
        //
        // We also build MaterialInfo for each material, so that meshes could quickly lookup
        // relevant indices based on MaterialIndex (from assimp).

        auto materials = std::span(scene->mMaterials, scene->mNumMaterials);
        for (auto material : materials) {

            auto get_full_path = [&](const aiMaterial& material, aiTextureType type) -> AssetPath {
                aiString filename;
                material.GetTexture(type, 0u, &filename);
                return AssetPath{ request.model.path.file.parent_path() / filename.C_Str(), {} };
            };

            auto get_texture_type = [&](ImageIntent intent) -> aiTextureType {
                switch (intent) {
                    case ImageIntent::Albedo:
                        return aiTextureType_DIFFUSE;
                    case ImageIntent::Specular:
                        return aiTextureType_SPECULAR;
                    case ImageIntent::Normal:
                        if (Assimp::BaseImporter::SimpleExtensionCheck(request.model.path.file, "obj")) {
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

            auto get_minmax_channels = [](ImageIntent intent) -> std::pair<size_t, size_t> {
                size_t min_channels = 0;
                size_t max_channels = 4;
                switch (intent) {
                    case ImageIntent::Albedo:
                        min_channels = 3;
                        max_channels = 4;
                        break;
                    case ImageIntent::Specular:
                        min_channels = 1;
                        max_channels = 1;
                        break;
                    case ImageIntent::Normal:
                        min_channels = 3;
                        max_channels = 3;
                        break;
                    case ImageIntent::Alpha: // NOLINT(bugprone-branch-clone)
                        min_channels = 1;
                        max_channels = 1;
                        break;
                    case ImageIntent::Heightmap:
                        min_channels = 1;
                        max_channels = 1;
                        break;
                    default:
                        break;
                };
                return { min_channels, max_channels };
            };

            // Will update `textures` and `tex_index_map` too.
            auto load_and_emplace_if_exists = [&](ImageIntent intent)
                -> TextureIndex
            {
                const aiTextureType type = get_texture_type(intent);

                const bool exists = material->GetTextureCount(type);

                if (exists) {
                    AssetPath path = get_full_path(*material, type);

                    // 1. Check local cache.
                    // TODO: Check that the intent does not differ?
                    auto maybe_new_index = textures.size();

                    auto [tex_index_it, was_emplaced] = tex_index_map.try_emplace(path, maybe_new_index);

                    if (was_emplaced) {
                        // Got to push_back either ImageDataAsset or StoredTextureAsset

                        // 2. Check global cache.
                        auto cached = [&, this]()
                            -> std::optional<StoredTextureAsset>
                        {
                            std::shared_lock texture_cache_read_lock{ texture_cache_mutex_ };

                            auto it = texture_cache_.find(path);
                            if (it != texture_cache_.end()) {
                                return it->second;
                            }
                            return std::nullopt;

                            // ~texture_cache_read_lock
                        }();

                        if (cached.has_value()) {
                            textures.emplace_back(std::move(*cached));
                        } else {
                            // 3. Load and emplace if not found.
                            auto [min_channels, max_channels] = get_minmax_channels(intent);
                            auto image_data = load_image_data_from_file<chan::UByte>(
                                File(path.file), min_channels, max_channels
                            );
                            textures.emplace_back(ImageDataAsset{
                                .path   = std::move(path),
                                .intent = intent,
                                .data   = std::move(image_data),
                            });
                        }
                        assert(maybe_new_index + 1 == textures.size());
                    }
                    // If it wasn't emplaced, then it was already there,
                    // we can get the index from `tex_index_it` either way.
                    return tex_index_it->second;
                } else {
                    return -1; // If no texture with such ImageIntent in the material.
                }

            };

            // Body of the loop here :^)

            TextureIndex diffuse_id  = load_and_emplace_if_exists(ImageIntent::Albedo);
            TextureIndex specular_id = load_and_emplace_if_exists(ImageIntent::Specular);
            TextureIndex normal_id   = load_and_emplace_if_exists(ImageIntent::Normal);

            material_info.emplace_back(MaterialInfo{
                .diffuse_id  = diffuse_id,
                .specular_id = specular_id,
                .normal_id   = normal_id,
            });

        }

        request.model.textures = std::move(textures);




        // Now the meshes themselves.


        auto get_mesh_data = [&v2v](const aiMesh& mesh)
            -> MeshData<VertexPNUTB>
        {
            auto verts      = std::span(mesh.mVertices,         mesh.mNumVertices);
            auto uvs        = std::span(mesh.mTextureCoords[0], mesh.mNumVertices);
            auto normals    = std::span(mesh.mNormals,          mesh.mNumVertices);
            auto tangents   = std::span(mesh.mTangents,         mesh.mNumVertices);
            auto bitangents = std::span(mesh.mBitangents,       mesh.mNumVertices);


            if (!normals.data()) {
                throw error::AssetContentsParsingError(
                    "Mesh data does not contain Normals");
            }

            if (!uvs.data()) {
                throw error::AssetContentsParsingError(
                    "Mesh data does not contain Texture Coordinates");
            }

            if (!tangents.data()) {
                throw error::AssetContentsParsingError(
                    "Mesh data does not contain Tangents");
            }

            if (!bitangents.data()) {
                throw error::AssetContentsParsingError(
                    "Mesh data does not contain Bitangents");
            }


            std::vector<VertexPNUTB> vertex_data;
            vertex_data.reserve(verts.size());

            for (size_t i{ 0 }; i < verts.size(); ++i) {
                vertex_data.emplace_back(VertexPNUTB{
                    .position  = v2v(verts[i]     ),
                    .normal    = v2v(normals[i]   ),
                    .uv        = v2v(uvs[i]       ),
                    .tangent   = v2v(tangents[i]  ),
                    .bitangent = v2v(bitangents[i]),
                });
            }


            auto faces = std::span(mesh.mFaces, mesh.mNumFaces);

            std::vector<unsigned int> indices;
            indices.reserve(faces.size() * 3);

            for (const auto& face : faces) {
                for (const auto& index : std::span(face.mIndices, face.mNumIndices)) {
                    indices.emplace_back(index);
                }
            }


            return { std::move(vertex_data), std::move(indices) };
        };


        auto meshes = std::span(scene->mMeshes, scene->mNumMeshes);
        for (const auto& mesh : meshes) {

            auto path      = AssetPath{ request.model.path.file, mesh->mName.C_Str() };
            auto aabb      = LocalAABB{ v2v(mesh->mAABB.mMin), v2v(mesh->mAABB.mMax) };
            auto mesh_data = get_mesh_data(*mesh);

            MeshDataAsset data_asset{
                .path = std::move(path),
                .aabb = std::move(aabb),
                .data = std::move(mesh_data),
            };

            auto mat_index       = mesh->mMaterialIndex;
            const auto& mat_info = material_info[mat_index];

            UnresolvedMesh unresolved{
                .mesh        = std::move(data_asset),
                .diffuse_id  = mat_info.diffuse_id,
                .specular_id = mat_info.specular_id,
                .normal_id   = mat_info.normal_id,
            };

            request.model.meshes.emplace_back(std::move(unresolved));
        }

    } catch (...) {
        set_exception(std::move(request.promise), std::current_exception());
        {
            std::scoped_lock pending_requests_lock{ pending_requests_mutex_ };
            auto it = pending_requests_.find(request.model.path);
            if (it != pending_requests_.end()) {
                for (auto& pending_promise : it->second) {
                    set_exception(std::move(pending_promise), std::current_exception());
                }
                pending_requests_.erase(it);
            }
        } // ~pending_requests_lock
        importer.FreeScene();
        return;
    }


    // Is this it?
    // Now propagate the request further.

    upload_requests_.emplace(std::move(request.model), std::move(request.promise));

    importer.FreeScene();
}








void AssetManager::handle_upload_request(UploadRequest&& request) {
    try {

        // Upload each material texture.
        //
        // We will either find an already cached `StoredTextureAsset`, or,
        // more likely, `ImageDataAsset` that needs to be uploaded to the GPU memory.

        for (UnresolvedTexture& texture : request.model.textures) {
            if (auto data_asset = std::get_if<ImageDataAsset>(&texture)) {


                // 0. Recheck Cache.
                {
                    std::shared_lock texture_cache_read_lock{ texture_cache_mutex_ };
                    // It could be that the same texture was already cached by another upload,
                    // But the Loading Thread could not find it yet by that time, so it loaded the texture again.
                    // We don't want duplicates to exist, so we drop this load here.

                    // Since only this thread writes to the cache, we can recheck it first, and only then
                    // try to upload a new texture, while releasing the lock in-between.

                    // TODO: Texture cache could have "pending" state to bypass this issue and make
                    // the Loading Thread skip redundant loads.

                    auto it = texture_cache_.find(data_asset->path);
                    if (it != texture_cache_.end()) {
                        texture.emplace<StoredTextureAsset>(it->second);
                        continue;
                    }
                } // ~texture_cache_read_lock



                // 1. Upload
                const auto num_channels = data_asset->data.num_channels();
                const auto intent       = data_asset->intent;

                const auto type   = PixelDataType::UByte;
                const auto format = [&]() -> PixelDataFormat {
                    switch (num_channels) {
                        case 1: return PixelDataFormat::Red;
                        case 2: return PixelDataFormat::RG;
                        case 3: return PixelDataFormat::RGB;
                        case 4: return PixelDataFormat::RGBA;
                        default: assert(false); return {};
                    }
                }();

                const auto iformat = [&]() -> InternalFormat {
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
                        default:
                            switch (num_channels) {
                                case 1: return InternalFormat::R8;
                                case 2: return InternalFormat::RG8;
                                case 3: return InternalFormat::RGB8;
                                case 4: return InternalFormat::RGBA8;
                                default: assert(false); return {};
                            }
                    }
                }();

                StoredTextureAsset asset{
                    .path    = std::move(data_asset->path),
                    .intent  = intent,
                    .texture = create_material_texture_from_image_data(data_asset->data, format, type, iformat),
                };



                // 2. Cache
                AssetPath          path_copy  = asset.path; // Copy outside of the lock.
                StoredTextureAsset asset_copy = asset;
                {
                    std::unique_lock texture_cache_write_lock{ texture_cache_mutex_ };

                    auto [it, was_emplaced] =
                        texture_cache_.emplace(std::move(path_copy), std::move(asset_copy));

                    // We are the only thread writing to cache, and we checked before for no cache.
                    assert(was_emplaced);
                } // ~texture_cache_write_lock



                // 3. Update
                texture.emplace<StoredTextureAsset>(std::move(asset));

            } // if (auto data_asset = std::get_if<ImageDataAsset>(&texture))
        } // for (MaterialTexture& texture : request.model.textures)


        // We'll be building this thing by uploading the Meshes
        // and linking them to their textures.
        StoredModelAsset stored_model{ request.model.path, {} };


        // Now upload the Meshes.

        for (auto& unresolved_mesh : request.model.meshes) {
            if (auto data_asset = std::get_if<MeshDataAsset>(&unresolved_mesh.mesh)) {

                // Upload Mesh Data.
                const auto& verts   = data_asset->data.vertices();
                const auto& indices = data_asset->data.elements();

                SharedBuffer<VertexPNUTB> verts_buf =
                    specify_buffer(std::span<const VertexPNUTB>(verts), StorageMode::StaticServer);

                SharedBuffer<GLuint> indices_buf =
                    specify_buffer(std::span<const GLuint>(indices), StorageMode::StaticServer);



                auto get_texture = [&](TextureIndex id) -> std::optional<StoredTextureAsset> {
                    if (id >= 0) {
                        // These should all be StoredTextureAsset at this point.
                        return std::get<StoredTextureAsset>(request.model.textures[id]);
                    } else /* no texture */ {
                        return std::nullopt;
                    }
                };


                StoredMeshAsset asset{
                    .path     = std::move(data_asset->path),
                    .aabb     = std::move(data_asset->aabb),
                    .vertices = std::move(verts_buf),
                    .indices  = std::move(indices_buf),
                    .diffuse  = get_texture(unresolved_mesh.diffuse_id),
                    .specular = get_texture(unresolved_mesh.specular_id),
                    .normal   = get_texture(unresolved_mesh.normal_id),
                };


                // Cache.
                {
                    std::scoped_lock mesh_cache_lock{ mesh_cache_mutex_ };
                    // Do we actually care about this? Whatever, cache it if it's not there.
                    auto [it, was_emplaced] =
                        mesh_cache_.try_emplace(asset.path, asset);
                } // ~mesh_cache_lock


                unresolved_mesh.mesh.emplace<StoredMeshAsset>(std::move(asset));
            }

            // Just a mess crap code at this point, whatever...

            auto& asset = std::get<StoredMeshAsset>(unresolved_mesh.mesh);

            stored_model.meshes.emplace_back(std::move(asset));
        }

        // Apparently, required.
        glapi::finish();


        // Cache the model and resolve all requests related to this asset..
        AssetPath path_copy = stored_model.path;
        {
            std::scoped_lock model_cache_lock{ model_cache_mutex_ };

            // We resolve the primary request under a cache lock too,
            // this forces the Dispatch Thread to wait until the lock is released,
            // before checking the cache.
            //
            // From the perspective of the caller, this guarantees that once
            // the initial load completes, any repeated requests for the same asset
            // will be sourced from the cache.
            //
            // This is not required and can be moved outside the lock
            // in exchange for the above guarantee.
            set_result(std::move(request.promise), stored_to_shared(stored_model));

            auto [it, was_emplaced] =
                model_cache_.try_emplace(std::move(path_copy), stored_model);

            // If there are any pending requests, resolve them.
            // Remove entry of this request from pending_requests_.
            {
                std::scoped_lock pending_requests_lock{ pending_requests_mutex_ };
                auto it = pending_requests_.find(request.model.path);
                assert(it != pending_requests_.end()); // Must be emplaced by the dispatch thread, even if empty.
                for (auto& pending_promise : it->second) {
                    set_result(std::move(pending_promise), stored_to_shared(stored_model));
                }
                pending_requests_.erase(it);
            } // ~pending_requests_lock
        } // ~model_cache_lock


    } catch (...) {
        set_exception(std::move(request.promise), std::current_exception());
        {
            std::scoped_lock pending_requests_lock{ pending_requests_mutex_ };
            auto it = pending_requests_.find(request.model.path);
            if (it != pending_requests_.end()) {
                for (auto& pending_promise : it->second) {
                    set_exception(std::move(pending_promise), std::current_exception());
                }
                pending_requests_.erase(it);
            }
        } // ~pending_requests_lock
        return;
    }

}



} // namespace josh
