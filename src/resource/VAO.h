#pragma once
#include <numeric>
#include <glad/glad.h>
#include "TypeAliases.h"
#include "IResource.h"
#include "VBO.h"

class VAO : public IResource {
private:
	size_t m_numVertices;

protected:
	void acquireResource() { glGenVertexArrays(1, &m_id); }
	void releaseResource() { glDeleteVertexArrays(1, &m_id); }

public:
	// for now this only takes one VBO 
	VAO(const VBO& vbo, GLenum usage = GL_STATIC_DRAW);

	void bind() const { glBindVertexArray(m_id); }
	static void unbind() { glBindVertexArray(0); }
	
	void draw(int firstOffset = 0, GLenum mode = GL_TRIANGLES) const {
		glDrawArrays(GL_TRIANGLES, firstOffset, static_cast<int>(m_numVertices)); 
	}
	
	void bindAndDraw(int firstOffset = 0, GLenum mode = GL_TRIANGLES) const {
		bind();
		draw();
	}


};


inline VAO::VAO(const VBO& vbo, GLenum usage) {

	acquireResource();
	bind();

	vbo.bind();
	const auto& data{ vbo.getData() };
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * data.size(), data.data(), usage);

	const auto& VALayout{ vbo.getLayout() };
	auto stride{ vbo.getStride() };
	auto offset{ 0 };
	for ( int i{ 0 }; const auto& currentLayout : VALayout ) {

		glVertexAttribPointer(currentLayout.index, currentLayout.size, GL_FLOAT, GL_FALSE,
		                      stride * sizeof(float), reinterpret_cast<void*>(offset * sizeof(float)));
		glEnableVertexAttribArray(currentLayout.index);

		offset += currentLayout.size;
		++i;
	}

	m_numVertices = data.size() / stride;
}

