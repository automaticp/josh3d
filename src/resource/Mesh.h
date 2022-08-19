#include <glm/glm.hpp>
#include <vector>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#include "Texture.h"
#include "ResourceAllocators.h"
#include <concepts>
#include <utility>
#include <array>
#include "ShaderProgram.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cstring>
#include <iostream>
#include <span>
#include <cassert>
#include <cstddef>
#include <variant>
#include <stdexcept>

struct AttributeParams {
    GLuint      index;
    GLint       size;
    GLenum      type;
    GLboolean   normalized;
    GLsizei     stride_bytes;
    GLint64       offset_bytes;
};


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_uv;
};


template<typename V>
struct VertexTraits {
    static_assert(sizeof(V) == 0, "Custom vertex class V must have a VertexTraits<V> specialization. There's no default that makes sense.");
    constexpr static std::array<AttributeParams, 1> aparams;
};

template<>
struct VertexTraits<Vertex> {
    constexpr static std::array<AttributeParams, 3> aparams{{
        { 0u, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position) },
        { 1u, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normal) },
        { 2u, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, tex_uv) }
    }};
};


namespace detail {

using namespace gl;

class BoundVBO;

class BoundVAO {
public:
    BoundVAO& enable_array_access(GLuint attrib_index) {
        glEnableVertexAttribArray(attrib_index);
        return *this;
    }

    BoundVAO& disable_array_access(GLuint attrib_index) {
        glDisableVertexAttribArray(attrib_index);
        return *this;
    }

    BoundVAO& draw_arrays(GLenum mode, GLint first, GLsizei count) {
        glDrawArrays(mode, first, count);
        return *this;
    }

    BoundVAO& draw_elements(GLenum mode, GLsizei count, GLenum type,
                       const void* indices_buffer = nullptr) {
        glDrawElements(mode, count, type, indices_buffer);
        return *this;
    }

    template<size_t N>
    BoundVAO& set_many_attribute_params(
        const std::array<AttributeParams, N>& aparams) {

        for (const auto& ap : aparams) {
            set_attribute_params(ap);
            this->enable_array_access(ap.index);
        }
        return *this;
    }



    template<size_t N>
    BoundVAO& associate_with(BoundVBO& vbo,
                        const std::array<AttributeParams, N>& aparams) {
        this->set_many_attribute_params(aparams);
        return *this;
    }


    static void set_attribute_params(const AttributeParams& ap) {
        glVertexAttribPointer(
            ap.index, ap.size, ap.type, ap.normalized,
            ap.stride_bytes, reinterpret_cast<const void*>(ap.offset_bytes)
        );
    }


    void unbind() {
        glBindVertexArray(0u);
    }

};

class VAO : public VAOAllocator {
public:
    BoundVAO bind() {
        glBindVertexArray(id_);
        return {};
    }
};




class BoundVBO {
public:
    template<typename T>
    BoundVBO& attach_data(size_t size, const T* data, GLenum usage) {
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data),
            usage
        );
        return *this;
    }

    template<size_t N>
    BoundVBO& associate_with(BoundVAO& vao,
                        const std::array<AttributeParams, N>& aparams) {
        vao.associate_with(*this, aparams);
        return *this;
    }

    void unbind() {
        glBindBuffer(GL_ARRAY_BUFFER, 0u);
    }
};

class VBO : public VBOAllocator {
public:
    BoundVBO bind() {
        glBindBuffer(GL_ARRAY_BUFFER, id_);
        return {};
    }
};




class BoundEBO {
public:
    template<typename T>
    BoundEBO& attach_data(size_t size, const T* data, GLenum usage) {
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(size * sizeof(T)),
            reinterpret_cast<const void*>(data),
            usage
        );
        return *this;
    }

    void unbind() {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);
    }
};

class EBO : public VBOAllocator {
public:
    BoundEBO bind(BoundVAO& vao) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
        return {};
    }
};



/*
VAO vao;
VBO vbo;
EBO ebo;
auto bvao = vao.bind();
vbo.bind().attach_data(...).associate_with(bvao, ...);
ebo.bind(bvao).attach_data(...);
*/



// Texture data variants

class StbImageData {
private:
    using stb_deleter_t = decltype(
        [](std::byte* data) {
            if (data) { stbi_image_free(reinterpret_cast<stbi_uc*>(data)); }
        }
    );
    using stb_image_owner_t = std::unique_ptr<std::byte[], stb_deleter_t>;

    size_t width_;
    size_t height_;
    size_t n_channels_;
    stb_image_owner_t data_{};

public:
    StbImageData(const std::string& path, int num_desired_channels = 0) {

        stbi_set_flip_vertically_on_load(true);

        int width, height, n_channels;
        data_.reset(
            reinterpret_cast<std::byte*>(
                stbi_load(path.c_str(), &width, &height, &n_channels, num_desired_channels)
            )
        );
        if (!data_) {
            throw std::runtime_error("Stb could not load the image at " + path);
        }

        this->width_      = width;
        this->height_     = height;
        this->n_channels_ = n_channels;
    }

    size_t size() const noexcept { return width_ * height_ * n_channels_; }
    std::byte* data() const noexcept { return data_.get(); }
    size_t width() const noexcept { return width_; }
    size_t height() const noexcept { return height_; }
    size_t n_channels() const noexcept { return n_channels_; }

};


class ImageData {
private:
    size_t width_;
    size_t height_;
    size_t n_channels_;
    std::unique_ptr<std::byte[]> data_;

public:
    ImageData(size_t width, size_t height, size_t n_channels)
        : width_{ width }, height_{ height }, n_channels_{ n_channels },
        data_{ std::make_unique_for_overwrite<std::byte[]>(size()) } {}

    size_t size() const noexcept { return width_ * height_ * n_channels_; }
    std::byte* data() const noexcept { return data_.get(); }
    size_t width() const noexcept { return width_; }
    size_t height() const noexcept { return height_; }
    size_t n_channels() const noexcept { return n_channels_; }

};


class TextureData : public std::variant<ImageData, StbImageData> {
public:
    size_t size() const noexcept { return std::visit([](auto&& v) { return v.size(); }, *this); }
    std::byte* data() const noexcept { return std::visit([](auto&& v) { return v.data(); }, *this); }
    size_t width() const noexcept { return std::visit([](auto&& v) { return v.width(); }, *this); }
    size_t height() const noexcept { return std::visit([](auto&& v) { return v.height(); }, *this); }
    size_t n_channels() const noexcept { return std::visit([](auto&& v) { return v.n_channels(); }, *this); }
};








class BoundTextureHandle {
public:

    BoundTextureHandle& attach_data(const TextureData& tex_data,
        GLenum internal_format = GL_RGBA, GLenum format = GL_NONE) {

        if (format == GL_NONE) {
            switch (tex_data.n_channels()) {
                case 1ull: format = GL_RED; break;
                case 2ull: format = GL_RG; break;
                case 3ull: format = GL_RGB; break;
                case 4ull: format = GL_RGBA; break;
                default: format = GL_RED;
            }
        }

        glTexImage2D(
            GL_TEXTURE_2D, 0, static_cast<GLint>(internal_format),
            tex_data.width(), tex_data.height(), 0,
            format, GL_UNSIGNED_BYTE, tex_data.data()
        );
        glGenerateMipmap(GL_TEXTURE_2D);

        return *this;
    }

};

class TextureHandle : public TextureAllocator {
public:
    BoundTextureHandle bind() {
        glBindTexture(GL_TEXTURE_2D, id_);
        return {};
    }

    BoundTextureHandle bind_to_unit(GLenum tex_unit) {
        set_active_unit(tex_unit);
        bind();
        return {};
    }

    static void set_active_unit(GLenum tex_unit) { glActiveTexture(tex_unit); }

};






} // namespace detail



template<typename V>
class Mesh {
private:
    std::vector<V> vertices;
    std::vector<GLuint> elements;

    // FIXME: separate texture handle from data
    detail::TextureHandle diffuse_texture;
    detail::TextureHandle specular_texture;

    detail::VBO vbo;
    detail::VAO vao;
    detail::EBO ebo;

public:
    Mesh(std::vector<V> vertices, std::vector<GLuint> elements,
         detail::TextureHandle diffuse, detail::TextureHandle specular)
        : vertices{ std::move(vertices) }, elements{ std::move(elements) },
        diffuse_texture{ std::move(diffuse) }, specular_texture{ std::move(specular) }
    {

        auto bvao = vao.bind();

        vbo.bind()
           .attach_data(vertices.size(), vertices.data(), GL_STATIC_DRAW)
           .associate_with(bvao, VertexTraits<V>::aparams);

        ebo.bind(bvao)
           .attach_data(elements.size(), elements.data(), GL_STATIC_DRAW);

    }

    void draw(ShaderProgram& sp) {

        sp.setUniform("material.diffuse", 0);
	    diffuse_texture.bind_to_unit(GL_TEXTURE0);

        sp.setUniform("material.specular", 1);
	    specular_texture.bind_to_unit(GL_TEXTURE1);

        sp.setUniform("material.shininess", 128.0f);


        vao.bind()
           .draw_elements(GL_TRIANGLES, elements.size(), GL_UNSIGNED_INT, nullptr);


    }


};



// Provide specialization for your own Vertex layout
template<typename V>
std::vector<V> get_vertex_data(const aiMesh* mesh);


template<typename V>
class Model {
private:
    // FIXME: bad design. But passing it through 5 function calls is worse.
    // Also keep this above meshes_ as initialization order matters here.
    std::string directory_;
    std::vector<Mesh<V>> meshes_;

public:

    void draw(ShaderProgram& sp) {
        for (auto& mesh : meshes_) {
            mesh.draw(sp);
        }
    }

    Model(Assimp::Importer& importer, const std::string& path)
        : directory_{ path.substr(0ull, path.find_last_of('/') + 1) },
        meshes_{ retrieve_mesh_data(importer, path) }
    {}


private:

    std::vector<Mesh<V>> retrieve_mesh_data(Assimp::Importer& importer,
                              const std::string& path)
    {
        const aiScene* scene{
            importer.ReadFile(path,
                aiProcess_Triangulate | aiProcess_FlipUVs |
                aiProcess_ImproveCacheLocality
            )
        };

        if ( std::strlen(importer.GetErrorString()) != 0 ) {
            std::cerr << "Assimp Error: " << importer.GetErrorString() << '\n';
            // FIXME: Throw maybe?
            // Myself. Out the window.
        }

        std::vector<Mesh<V>> meshes;
        meshes.reserve(scene->mNumMeshes);
        process_node(scene->mRootNode, scene, meshes);

        return meshes;
    }

    void process_node(aiNode* node, const aiScene* scene, std::vector<Mesh<V>>& meshes) {

        for ( auto&& mesh_id : std::span(node->mMeshes, node->mNumMeshes) ) {
            aiMesh* mesh{ scene->mMeshes[mesh_id] };
            meshes.emplace_back(get_mesh_data(mesh, scene));
        }

        for ( auto&& child : std::span(node->mChildren, node->mNumChildren) ) {
            process_node(child, scene, meshes);
        }
    }



    Mesh<V> get_mesh_data(const aiMesh* mesh, const aiScene* scene) {
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material{ scene->mMaterials[mesh->mMaterialIndex] };
            detail::TextureHandle diffuse  = get_texture_from_material(material, aiTextureType_DIFFUSE);
            detail::TextureHandle specular = get_texture_from_material(material, aiTextureType_SPECULAR);
            return Mesh<V>(
                get_vertex_data<V>(mesh), get_element_data(mesh), std::move(diffuse), std::move(specular)
            );
        } else {
            throw std::runtime_error("The requested mesh has no valid material index");
        }
    }


    detail::TextureHandle get_texture_from_material(const aiMaterial* material, aiTextureType type) {

        assert(material->GetTextureCount(type) == 1);

        aiString filename;
        material->GetTexture(type, 0ull, &filename);

        detail::TextureData tex_data{ detail::StbImageData(directory_ + filename.C_Str()) };
        detail::TextureHandle tex;

        // FIXME: this is ewww
        gl::GLenum tex_unit;
        switch (type) {
            case aiTextureType_SPECULAR: tex_unit = GL_TEXTURE1; break;
            default:
            case aiTextureType_DIFFUSE:  tex_unit = GL_TEXTURE0; break;
        }

        tex.bind_to_unit(tex_unit).attach_data(tex_data);
        return tex;
    }


    static std::vector<GLuint> get_element_data(const aiMesh* mesh) {
        std::vector<GLuint> indices;
        // FIXME: Is there a way to reserve correctly?
        indices.reserve(mesh->mNumFaces * 3ull);
        for ( const auto& face : std::span<aiFace>(mesh->mFaces, mesh->mNumFaces) ) {
            for ( const auto& index : std::span<GLuint>(face.mIndices, face.mNumIndices) ) {
                indices.emplace_back(index);
            }
        }
        return indices;
    }

};



template<>
inline std::vector<Vertex> get_vertex_data(const aiMesh* mesh) {
    std::span<aiVector3D> positions( mesh->mVertices, mesh->mNumVertices );
    std::span<aiVector3D> normals( mesh->mNormals, mesh->mNumVertices );
    // Why texture coordinates are in a 3D space?
    std::span<aiVector3D> tex_uvs( mesh->mTextureCoords[0], mesh->mNumVertices );

    if ( !mesh->mNormals || !mesh->mTextureCoords[0] ) {
        assert(false);
        // FIXME: Throw maybe?
        // Violates assumptions about the contents of Vertex.
    }

    std::vector<Vertex> vertices;
    vertices.reserve(mesh->mNumVertices);
    for ( size_t i{ 0 }; i < mesh->mNumVertices; ++i ) {
        vertices.emplace_back(
            glm::vec3{ positions[i].x,  positions[i].y,  positions[i].z },
            glm::vec3{ normals[i].x,    normals[i].y,    normals[i].z   },
            glm::vec2{ tex_uvs[i].x,    tex_uvs[i].y    }
        );
    }

    return vertices;
}

