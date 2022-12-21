
#include "Rendering/Vulkan/Helpers/VKVertex.hpp"

namespace Puffin::Rendering::VKUtil
{
	VertexBuilder VertexBuilder::Begin()
	{
		VertexBuilder builder;
		return builder;
	}

	VertexBuilder& VertexBuilder::BindSize(uint32_t size)
	{
		m_bindingDescription = {};
		m_bindingDescription.binding = 0;
		m_bindingDescription.stride = size;
		m_bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return *this;
	}

	VertexBuilder& VertexBuilder::BindAttribute(VkFormat format, uint32_t offset)
	{
		VkVertexInputAttributeDescription attributeDescription = {};

		attributeDescription.binding = 0;
		attributeDescription.location = static_cast<uint32_t>(m_attributeDescriptions.size());
		attributeDescription.format = format;
		attributeDescription.offset = offset;

		m_attributeDescriptions.push_back(attributeDescription);

		return *this;
	}

	bool VertexBuilder::Build(VkVertexInputBindingDescription& bindingDescription,
		std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
	{
		bindingDescription = m_bindingDescription;
		attributeDescriptions = m_attributeDescriptions;

		return true;
	}
}
