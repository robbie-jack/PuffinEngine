#pragma once

#include <vulkan/vulkan.hpp>

#include "puffin/types/vertex.h"

namespace puffin::rendering::util
{
	class VertexLayout
	{
	public:

		static VertexLayout Begin();

		VertexLayout& BindInput(uint32_t binding, uint32_t stride);

		VertexLayout& BindAttribute(uint32_t location, uint32_t binding, vk::Format format, uint32_t offset);

		[[nodiscard]] std::vector<vk::VertexInputBindingDescription> Bindings() const;
		[[nodiscard]] std::vector <vk::VertexInputAttributeDescription> Attributes() const;

	private:

		std::vector<vk::VertexInputBindingDescription> mBindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> mAttributeDescriptions;

	};
}

namespace puffin::rendering
{
	inline util::VertexLayout VertexPC32::GetLayoutVK()
	{
		return util::VertexLayout::Begin()
			.BindInput(0, sizeof(VertexPC32))
			.BindAttribute(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPC32, pos))
			.BindAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPC32, color));
	}

	inline util::VertexLayout VertexPNC32::GetLayoutVK()
	{
		return util::VertexLayout::Begin()
			.BindInput(0, sizeof(VertexPNC32))
			.BindAttribute(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNC32, pos))
			.BindAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNC32, normal))
			.BindAttribute(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNC32, color));
	}

	inline util::VertexLayout VertexPNTV32::GetLayoutVK()
	{
		return util::VertexLayout::Begin()
			.BindInput(0, sizeof(VertexPNTV32))
			.BindAttribute(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNTV32, pos))
			.BindAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNTV32, normal))
			.BindAttribute(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNTV32, tangent))
			/*.bindAttribute(3, 0, vk::Format::eR32G32Sfloat, offsetof(VertexPNTV32, uv))*/;
	}

	inline util::VertexLayout VertexP64NTV32::GetLayoutVK()
	{
		return util::VertexLayout::Begin()
			.BindInput(0, sizeof(VertexP64NTV32))
			.BindAttribute(0, 0, vk::Format::eR64G64B64Sfloat, offsetof(VertexP64NTV32, pos))
			.BindAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexP64NTV32, normal))
			.BindAttribute(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexP64NTV32, tangent))
			.BindAttribute(3, 0, vk::Format::eR32G32Sfloat, offsetof(VertexP64NTV32, uv));
	}
}