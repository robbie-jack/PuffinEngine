#pragma once

#include <vulkan/vulkan.hpp>

#include "Types/Vertex.hpp"

namespace Puffin::Rendering::VK::Util
{
	class VertexLayout
	{
	public:

		static VertexLayout Begin()
		{
			VertexLayout layout;
			return layout;
		}

		VertexLayout& BindInput(uint32_t binding, uint32_t stride)
		{
			m_bindingDescriptions.emplace_back(binding, stride);

			return *this;
		}

		VertexLayout& BindAttribute(uint32_t location, uint32_t binding, vk::Format format, uint32_t offset)
		{
			m_attributeDescriptions.emplace_back(location, binding, format, offset);

			return *this;
		}

		[[nodiscard]] std::vector<vk::VertexInputBindingDescription> Bindings() const { return m_bindingDescriptions; }
		[[nodiscard]] std::vector <vk::VertexInputAttributeDescription> Attributes() const { return m_attributeDescriptions; }

	private:

		std::vector<vk::VertexInputBindingDescription> m_bindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> m_attributeDescriptions;

	};
}

namespace Puffin::Rendering
{
	inline VK::Util::VertexLayout VertexPNC32::GetLayoutVK()
	{
		return VK::Util::VertexLayout::Begin()
			.BindInput(0, sizeof(VertexPNC32))
			.BindAttribute(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNC32, pos))
			.BindAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNC32, normal))
			.BindAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNC32, color));
	}

	inline VK::Util::VertexLayout VertexPNTV32::GetLayoutVK()
	{
		return VK::Util::VertexLayout::Begin()
			.BindInput(0, sizeof(VertexPNTV32))
			.BindAttribute(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNTV32, pos))
			.BindAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNTV32, normal))
			.BindAttribute(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNTV32, tangent))
			.BindAttribute(3, 0, vk::Format::eR32G32Sfloat, offsetof(VertexPNTV32, uv));
	}

	inline VK::Util::VertexLayout VertexP64NTV32::GetLayoutVK()
	{
		return VK::Util::VertexLayout::Begin()
			.BindInput(0, sizeof(VertexP64NTV32))
			.BindAttribute(0, 0, vk::Format::eR64G64B64Sfloat, offsetof(VertexP64NTV32, pos))
			.BindAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexP64NTV32, normal))
			.BindAttribute(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexP64NTV32, tangent))
			.BindAttribute(3, 0, vk::Format::eR32G32Sfloat, offsetof(VertexP64NTV32, uv));
	}
}