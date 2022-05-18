#pragma once
#include <numeric>
#include <glbinding/gl/gl.h>
#include "TypeAliases.h"
#include "VBO.h"
#include "ResourceAllocators.h"

using namespace gl;


class VAO : public VAOAllocator {
private:
	size_t numVertices_;

public:
	// for now this only takes one VBO
	explicit VAO(const VBO& vbo, GLenum usage = GL_STATIC_DRAW);

	void bind() const { glBindVertexArray(id_); }
	static void unbind() { glBindVertexArray(0); }

	void draw(int firstOffset = 0, GLenum mode = GL_TRIANGLES) const {
		glDrawArrays(mode, firstOffset, static_cast<int>(numVertices_));
	}

	void bindAndDraw(int firstOffset = 0, GLenum mode = GL_TRIANGLES) const {
		bind();
		draw(firstOffset, mode);
	}


};


inline VAO::VAO(const VBO& vbo, GLenum usage) {

	bind();

	vbo.bind();
	const auto& data{ vbo.getData() };
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(float) * data.size()), data.data(), usage);

	const auto& VALayout{ vbo.getLayout() };
	auto stride{ vbo.getStride() };
	auto offset{ 0 };
	for ( int i{ 0 }; const auto& currentLayout : VALayout ) {

		glVertexAttribPointer(currentLayout.index, currentLayout.size, GL_FLOAT, GL_FALSE,
		                      stride * static_cast<VertexAttributeLayout::AttribSize_t>(sizeof(float)),
							  reinterpret_cast<void*>(offset * sizeof(float)));
		glEnableVertexAttribArray(currentLayout.index);

		offset += currentLayout.size;
		++i;
	}

	numVertices_ = data.size() / stride;
}

