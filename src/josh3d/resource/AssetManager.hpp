#pragma once
#include "Channels.hpp"
#include "Filesystem.hpp"
#include "AABB.hpp"
#include "Future.hpp"
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "ImageData.hpp"
#include "MeshData.hpp"
#include "ThreadsafeQueue.hpp"
#include "VPath.hpp"
#include "VertexPNUTB.hpp"
#include "VirtualFilesystem.hpp"
#include <glbinding/gl/types.h>
#include <cstdint>
#include <stop_token>
#include <unordered_map>
#include <variant>
#include <mutex>
#include <shared_mutex>
#include <thread>




namespace glfw { class Window; }


namespace josh {


// ImageIntent affects the number of channels
// and the internal format of loaded textures.
enum class ImageIntent {
    Unknown,
    Albedo,
    Alpha,
    Specular,
    Normal,
    Heightmap,
};




struct AssetVPath {
    VPath       file;
    std::string subpath;

    bool operator==(const AssetVPath& other) const noexcept = default;
    std::strong_ordering operator<=>(const AssetVPath& other) const noexcept = default;
};

struct AssetPath {
    Path        file;
    std::string subpath;

    bool operator==(const AssetPath& other) const noexcept = default;
    std::strong_ordering operator<=>(const AssetPath& other) const noexcept = default;
};


} // namespace josh
namespace std {
template<> struct hash<josh::AssetPath> {
    std::size_t operator()(const josh::AssetPath& value) const noexcept {
        // This is probably terrible, but...
        const auto phash = hash<josh::Path>{}(value.file);
        const auto shash = hash<std::string>{}(value.subpath);
        return phash ^ (shash << 1);
        // I really don't care.
    }
};
} // namespace std
namespace josh {







namespace error {


class AssetLoadingError : public RuntimeError {
public:
    static constexpr auto prefix = "Asset Loading Error: ";
    AssetLoadingError(std::string msg)
        : AssetLoadingError(prefix, std::move(msg))
    {}
protected:
    AssetLoadingError(const char* prefix, std::string msg)
        : RuntimeError(prefix, std::move(msg))
    {}
};


class AssetFileImportFailure : public AssetLoadingError {
public:
    static constexpr auto prefix = "Asset File Import Failure: ";
    Path path;
    AssetFileImportFailure(Path path, std::string error_string)
        : AssetFileImportFailure(prefix,
            std::move(path), std::move(error_string))
    {}
protected:
    AssetFileImportFailure(const char* prefix,
        Path path, std::string error_string)
        : AssetLoadingError(prefix, std::move(error_string))
        , path{ std::move(path) }
    {}
};


class AssetContentsParsingError : public AssetLoadingError {
public:
    static constexpr auto prefix = "Asset Contents Parsing Error: ";
    AssetContentsParsingError(std::string msg)
        : AssetContentsParsingError(prefix, std::move(msg))
    {}
protected:
    AssetContentsParsingError(const char* prefix, std::string msg)
        : AssetLoadingError(prefix, std::move(msg))
    {}
};


} // namespace error










struct SharedTextureAsset {
    AssetPath            path;
    ImageIntent          intent;
    SharedConstTexture2D texture;
};

struct SharedMeshAsset {
    AssetPath                         path;
    LocalAABB                         aabb;
    SharedConstBuffer<VertexPNUTB>    vertices;
    SharedConstBuffer<GLuint>         indices;
    std::optional<SharedTextureAsset> diffuse;
    std::optional<SharedTextureAsset> specular;
    std::optional<SharedTextureAsset> normal;
};

struct SharedModelAsset {
    AssetPath                    path;
    std::vector<SharedMeshAsset> meshes;
};








// The name is chosen purely for aesthetic reasons.
class AssetManager {
public:
    AssetManager(VirtualFilesystem& vfs, const glfw::Window& shared_context);

    // Request an asynchronous load of a resource at `path`.
    //
    // Resulting resources must be made visible in the rendering
    // thread by binding them to that thread's context before use.
    auto load_model(const AssetPath&   path) -> Future<SharedModelAsset>;

    // Request an asynchronous load of a resource at `vpath` as
    // resolved through the referenced VirtualFilesystem.
    //
    // Resulting resources must be made visible in the rendering
    // thread by binding them to that thread's context before use.
    auto load_model(const AssetVPath& vpath) -> Future<SharedModelAsset>;



private:
    // For path resolution, used by Main Thread.
    VirtualFilesystem& vfs_;


    struct StoredTextureAsset {
        AssetPath       path;
        ImageIntent     intent;
        SharedTexture2D texture;
    };

    struct StoredMeshAsset {
        AssetPath                         path;
        LocalAABB                         aabb;
        SharedBuffer<VertexPNUTB>         vertices;
        SharedBuffer<GLuint>              indices;
        std::optional<StoredTextureAsset> diffuse;
        std::optional<StoredTextureAsset> specular;
        std::optional<StoredTextureAsset> normal;
    };

    struct StoredModelAsset {
        AssetPath                    path;
        std::vector<StoredMeshAsset> meshes;
    };

    static auto stored_to_shared(const StoredTextureAsset& stored) -> SharedTextureAsset;
    static auto stored_to_shared(const StoredMeshAsset&    stored) -> SharedMeshAsset;
    static auto stored_to_shared(const StoredModelAsset&   stored) -> SharedModelAsset;






    std::unordered_map<AssetPath, StoredTextureAsset> texture_cache_;
    std::unordered_map<AssetPath, StoredMeshAsset>    mesh_cache_;
    std::unordered_map<AssetPath, StoredModelAsset>   model_cache_;
    std::shared_mutex                                 texture_cache_mutex_;
    std::mutex                                        mesh_cache_mutex_; // Unused?
    std::mutex                                        model_cache_mutex_;



    struct ImageDataAsset {
        AssetPath               path;
        ImageIntent             intent;
        ImageData<chan::UByte>  data;
    };

    struct MeshDataAsset {
        AssetPath             path;
        LocalAABB             aabb;
        MeshData<VertexPNUTB> data;
    };





    // Mesh and Texture assets are tiny state-machines
    // with only 2 states: CPU-Data and GPU-Data.
    //
    // If the Model is not cached, then each new load request
    // is resolved in the following way:
    //
    // 1. Obtain GPU-Data for each asset by either loading CPU-Data
    // from disk and uploading it to the GPU, or by using a cached version.
    //
    // 2. Link Meshes to their Textures by using their TextureIndex,
    // and compose StoredModelAsset from all the Meshes.
    //
    // 3. Cache StoredModelAsset.
    //
    // 4. Resolve the requests by providing a SharedModelAsset from
    // the StoredModelAsset, which is mainly an immutable access to
    // the same resources.


    using TextureIndex = int32_t; // -1 for "no texture"

    struct UnresolvedMesh {
        std::variant<MeshDataAsset, StoredMeshAsset> mesh;

        TextureIndex diffuse_id  = -1;
        TextureIndex specular_id = -1;
        TextureIndex normal_id   = -1;
    };

    using UnresolvedTexture = std::variant<ImageDataAsset, StoredTextureAsset>;

    struct UnresolvedModel {
        AssetPath                      path;
        std::vector<UnresolvedMesh>    meshes;
        std::vector<UnresolvedTexture> textures;
    };




    // We operate in terms of 4 threads:
    //
    // 1. Main Thread - Thread interacting with the public interface of the class, could
    // be the rendering thread, but not required.
    //
    // 2. Dispatch Thread - Internal thread responsible for initial processing of load requests,
    // including resolving requests from cache and managing repeated pending requests.
    //
    // 3. Load Thread - Internal thread responsible for loading asset data from the disk.
    //
    // 4. Upload Thread - Internal thread responsible for uploading asset data to the gpu
    // and resolving active requests. Needs a separate loading context.


    std::unordered_map<AssetPath, std::vector<Promise<SharedModelAsset>>> pending_requests_;
    std::mutex                                                            pending_requests_mutex_;


    struct DispatchRequest {
        AssetPath                 path;
        Promise<SharedModelAsset> promise;
    };

    ThreadsafeQueue<DispatchRequest> dispatch_requests_;

    std::jthread dispatch_thread_;

    void dispatch_thread_loop(std::stop_token stoken);
    void handle_dispatch_request(DispatchRequest&& request);



    struct LoadRequest {
        UnresolvedModel           model;
        Promise<SharedModelAsset> promise;
    };

    ThreadsafeQueue<LoadRequest> load_requests_;

    std::jthread loading_thread_;

    void loading_thread_loop(std::stop_token stoken);
    void handle_load_request(LoadRequest&& request);



    struct UploadRequest {
        UnresolvedModel           model;
        Promise<SharedModelAsset> promise;
    };

    ThreadsafeQueue<UploadRequest> upload_requests_;

    std::jthread uploading_thread_;

    void uploading_thread_loop(std::stop_token stoken);
    void handle_upload_request(UploadRequest&& request);


};





} // namespace josh
