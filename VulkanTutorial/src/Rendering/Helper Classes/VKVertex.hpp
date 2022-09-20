#pragma once

#include <vulkan/vulkan.h>

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

		VkVertexInputBindingDescription m_bindingDescription;
		std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;

	};
}