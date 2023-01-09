#pragma once

#include "vulkan/vulkan.h"

#include "Types/Vertex.hpp"

// STL
#include <vector>


namespace Puffin::Rendering::VKUtil
{
	// Class to help with building vertices with custom attributes
	class VertexBuilder
	{
	public:

		static VertexBuilder Begin();

		VertexBuilder& BindSize(uint32_t size);

		VertexBuilder& BindAttribute(VkFormat format, uint32_t offset);

		bool Build(VkVertexInputBindingDescription& bindingDescription, std::vector<VkVertexInputAttributeDescription>& attributeDescriptions);

	private:

		VkVertexInputBindingDescription m_bindingDescription = {};
		std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;

	};
}

namespace Puffin::Rendering
{
	inline void VertexPC32::GetVertexBindingAndAttributes(VkVertexInputBindingDescription& bindingDescription, std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
	{
		VKUtil::VertexBuilder::Begin()
			.BindSize(sizeof(VertexPC32))
			.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPC32, pos))
			.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPC32, color))
			.Build(bindingDescription, attributeDescriptions);
	}

	inline void VertexPNTV32::GetVertexBindingAndAttributes(VkVertexInputBindingDescription& bindingDescription, std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
	{
		VKUtil::VertexBuilder::Begin()
			.BindSize(sizeof(VertexPNTV32))
			.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPNTV32, pos))
			.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPNTV32, normal))
			.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPNTV32, tangent))
			.BindAttribute(VK_FORMAT_R32G32_SFLOAT, offsetof(VertexPNTV32, uv))
			.Build(bindingDescription, attributeDescriptions);
	}

	inline void VertexP64NTV32::GetVertexBindingAndAttributes(VkVertexInputBindingDescription& bindingDescription, std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
	{
		VKUtil::VertexBuilder::Begin()
			.BindSize(sizeof(VertexP64NTV32))
			.BindAttribute(VK_FORMAT_R64G64B64_SFLOAT, offsetof(VertexP64NTV32, pos))
			.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexP64NTV32, normal))
			.BindAttribute(VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexP64NTV32, tangent))
			.BindAttribute(VK_FORMAT_R32G32_SFLOAT, offsetof(VertexP64NTV32, uv))
			.Build(bindingDescription, attributeDescriptions);
	}
}