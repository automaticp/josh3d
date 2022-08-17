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


struct AttributeParams {
    GLuint      index;
    GLint       size;
    GLenum      type;
    GLboolean   normalized;
    GLsizei     stride_bytes;
    GLint       offset_bytes;
};


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_uv;
};


template<typename V>
struct VertexTraits {
    static_assert(sizeof(V) == 0, "Custom vertex class must have a VertexTraits<> specialization. There's no default that makes sense.");
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

class VAO;
class VBO;
class EBO;
class BoundVAO;
class BoundVBO;
class BoundEBO;

class VAO : public VAOAllocator {
public:
    BoundVAO bind() {
        glBindVertexArray(id_);
        return {};
    }
};

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
            ap.stride_bytes, reinterpret_cast<void*>(ap.offset_bytes)
        );
    }

};




class VBO : public VBOAllocator {
public:
    BoundVBO bind() {
        glBindBuffer(GL_ARRAY_BUFFER, id_);
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
};




class EBO : public VBOAllocator {
public:
    BoundEBO bind(BoundVAO& vao) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
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
};


/*
VAO vao;
VBO vbo;
EBO ebo;
auto bvao = vao.bind();
vbo.bind().attach_data(...).associate_with(bvao, ...);
ebo.bind(bvao).attach_data(...);
*/

} // namespace detail



template<typename V>
class Mesh {
private:
    std::vector<V> vertices;
    std::vector<GLuint> elements;

    Texture diffuse_texture;
    Texture specular_texture;

    detail::VBO vbo;
    detail::VAO vao;
    detail::EBO ebo;

public:
    Mesh(std::vector<V> vertices, std::vector<GLuint> elements,
         const std::string& diffuse_filename,
         const std::string& specular_filename)
        : vertices{ std::move(vertices) }, elements{ std::move(elements) },
        diffuse_texture{ diffuse_filename },
        specular_texture{ specular_filename }
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
	    diffuse_texture.setActiveUnitAndBind(0);

        sp.setUniform("material.specular", 1);
	    specular_texture.setActiveUnitAndBind(1);

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
    std::vector<Mesh<V>> meshes;

public:

    void draw(ShaderProgram& sp) {
        for (auto& mesh : meshes) {
            mesh.draw(sp);
        }
    }


    Model& retrieve_mesh_data(Assimp::Importer& importer,
                              const std::string& path)
    {
        const aiScene* scene{
            importer.ReadFile(path,
                aiProcess_Triangulate || aiProcess_FlipUVs ||
                aiProcess_ImproveCacheLocality
            )
        };

        if ( std::strlen(importer.GetErrorString()) != 0 ) {
            std::cerr << "Assimp Error: " << importer.GetErrorString() << '\n';
            // FIXME: Throw maybe?
            // Myself. Out the window.
        }



        return *this;
    }

    Mesh get_mesh_data(const aiMesh* mesh) {
        //return Mesh(get_vertex_data<V>(mesh), get_element_data(mesh), ...);
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

