#pragma once

#include <vulkan/vulkan.hpp>

#include "Types/Vertex.hpp"

namespace Puffin::Rendering::VK::Util
{
	class VertexBuilder
	{
	public:

		static VertexBuilder Begin()
		{
			VertexBuilder builder;
			return builder;
		}

		VertexBuilder& BindInput(uint32_t size, uint32_t binding = 0)
		{
			m_bindingDescription = { binding, size };

			return *this;
		}

		VertexBuilder& BindAttribute(vk::Format format, uint32_t offset)
		{
			vk::VertexInputAttributeDescription attributeDescription = 
			{
				static_cast<uint32_t>(m_attributeDescriptions.size()),
				0, format, offset
			};

			m_attributeDescriptions.push_back(attributeDescription);

			return *this;
		}

		bool Build(vk::VertexInputBindingDescription& bindingDescription, std::vector<vk::VertexInputAttributeDescription>& attributeDescriptions)
		{
			bindingDescription = m_bindingDescription;
			attributeDescriptions = m_attributeDescriptions;

			return true;
		}

	private:

		vk::VertexInputBindingDescription m_bindingDescription = {};
		std::vector<vk::VertexInputAttributeDescription> m_attributeDescriptions;

	};
}

namespace Puffin::Rendering
{
	/*inline void GetVertexBindingAndAttributes(vk::VertexInputBindingDescription& bindingDescription,
		std::vector<vk::VertexInputAttributeDescription>& attributeDescriptions);*/

	inline void VertexPNC32::GetVertexBindingAndAttributes(vk::VertexInputBindingDescription& bindingDescription,
		std::vector<vk::VertexInputAttributeDescription>& attributeDescriptions)
	{
		VK::Util::VertexBuilder::Begin()
			.BindInput(sizeof(VertexPNC32))
			.BindAttribute(vk::Format::eR32G32B32Sfloat, offsetof(VertexPNC32, pos))
			.BindAttribute(vk::Format::eR32G32B32Sfloat, offsetof(VertexPNC32, normal))
			.BindAttribute(vk::Format::eR32G32B32Sfloat, offsetof(VertexPNC32, color))
			.Build(bindingDescription, attributeDescriptions);
	}
}