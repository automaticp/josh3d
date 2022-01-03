#pragma once
#include <array>
#include <vector>
#include <tuple>
#include <glad/glad.h>
#include "TypeAliases.h"
#include "glResource.h"

struct VertexAttributeLayout {
	using AttribIndex_t = int;
	using AttribSize_t = int;

	AttribIndex_t index;
	AttribSize_t size;
};


class VBO : public glResource {
private:
	std::vector<float> m_data;
	std::vector<VertexAttributeLayout> m_attributeLayout;

protected:
	virtual void acquireResource() override { glGenBuffers(1, &m_id); }
	virtual void releaseResource() override { glDeleteBuffers(1, &m_id); }

public:
	// somebody tell this man how to use forwarding constructors
	VBO(const std::vector<float>& data, const std::vector<VertexAttributeLayout>& attrbuteLayout)
		: m_data{ data }, m_attributeLayout{ attrbuteLayout } {
		
		acquireResource();
	}

	VBO(std::vector<float>&& data, const std::vector<VertexAttributeLayout>& attrbuteLayout)
		: m_data{ std::move(data) }, m_attributeLayout{ attrbuteLayout } {
	
		acquireResource();
	}

	VBO(std::vector<float>&& data, std::vector<VertexAttributeLayout>&& attrbuteLayout)
		: m_data{ std::move(data) }, m_attributeLayout{ std::move(attrbuteLayout) } {

		acquireResource();
	}


	const std::vector<VertexAttributeLayout>& getLayout() const noexcept { return m_attributeLayout; }
	const std::vector<float>& getData() const noexcept { return m_data; }

	const VertexAttributeLayout::AttribSize_t getStride() const {
		VertexAttributeLayout::AttribSize_t sum{ 0 };
		for ( const auto& currentLayout : m_attributeLayout ) {
			sum += currentLayout.size;
		}
		return sum;
	}

	const VertexAttributeLayout::AttribSize_t getOffset(int index) const {
		assert(index < m_attributeLayout.size());
		VertexAttributeLayout::AttribSize_t sum{ 0 };
		for ( int i{ 0 }; i < index; ++i ) {
			sum += m_attributeLayout[i].size;
		}
		return sum;
	}

	// why did this not have a bind method before???
	void bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_id); }
};

