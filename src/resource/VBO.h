#pragma once
#include <array>
#include <vector>
#include <utility>
#include <tuple>
#include <glad/glad.h>
#include "TypeAliases.h"
#include "IResource.h"

struct VertexAttributeLayout {
	using AttribIndex_t = GLsizei; // int
	using AttribSize_t = GLsizei;

	AttribIndex_t index;
	AttribSize_t size;
};


class VBOAllocator : public IResource {
protected:
	VBOAllocator() noexcept { glGenBuffers(1, &id_); }

public:
	virtual ~VBOAllocator() override { glDeleteBuffers(1, &id_); }
};




class VBO : public IResource {
private:
	std::vector<float> data_;
	std::vector<VertexAttributeLayout> attributeLayout_;

	void acquireResource() noexcept { glGenBuffers(1, &id_); }
	void releaseResource() noexcept { glDeleteBuffers(1, &id_); }

public:
	VBO(std::vector<float> data, std::vector<VertexAttributeLayout> attributeLayout)
			: data_{ std::move(data) }, attributeLayout_{ std::move(attributeLayout) } {
		acquireResource();
	}

	virtual ~VBO() override { releaseResource(); }

	const std::vector<VertexAttributeLayout>& getLayout() const noexcept { return attributeLayout_; }
	const std::vector<float>& getData() const noexcept { return data_; }

	const VertexAttributeLayout::AttribSize_t getStride() const;

	const VertexAttributeLayout::AttribSize_t getOffset(VertexAttributeLayout::AttribIndex_t index) const;

	// why did this not have a bind method before???
	void bind() const { glBindBuffer(GL_ARRAY_BUFFER, id_); }
};


inline const VertexAttributeLayout::AttribSize_t VBO::getStride() const {
	VertexAttributeLayout::AttribSize_t sum{ 0 };
	for ( const auto& currentLayout : attributeLayout_ ) {
		sum += currentLayout.size;
	}
	return sum;
}


inline const VertexAttributeLayout::AttribSize_t VBO::getOffset(VertexAttributeLayout::AttribIndex_t index) const {
	assert(index < attributeLayout_.size());
	VertexAttributeLayout::AttribSize_t sum{ 0 };
	for ( int i{ 0 }; i < index; ++i ) {
		sum += attributeLayout_[i].size;
	}
	return sum;
}

