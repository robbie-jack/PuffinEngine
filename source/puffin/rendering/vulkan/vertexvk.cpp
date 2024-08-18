#include "puffin/rendering/vulkan/vertexvk.h"

namespace puffin::rendering::util
{
	VertexLayout VertexLayout::Begin()
	{
		VertexLayout layout;
		return layout;
	}

	VertexLayout& VertexLayout::BindInput(uint32_t binding, uint32_t stride)
	{
		mBindingDescriptions.emplace_back(binding, stride);

		return *this;
	}

	VertexLayout& VertexLayout::BindAttribute(uint32_t location, uint32_t binding, vk::Format format, uint32_t offset)
	{
		mAttributeDescriptions.emplace_back(location, binding, format, offset);

		return *this;
	}

	std::vector<vk::VertexInputBindingDescription> VertexLayout::Bindings() const
	{
		return mBindingDescriptions;
	}

	std::vector<vk::VertexInputAttributeDescription> VertexLayout::Attributes() const
	{
		return mAttributeDescriptions;
	}
}
