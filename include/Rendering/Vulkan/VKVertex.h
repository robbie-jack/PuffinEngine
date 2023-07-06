#pragma once

#include <vulkan/vulkan.hpp>

#include "Types/Vertex.h"

namespace puffin::rendering::util
{
	class VertexLayout
	{
	public:

		static VertexLayout begin()
		{
			VertexLayout layout;
			return layout;
		}

		VertexLayout& bindInput(uint32_t binding, uint32_t stride)
		{
			mBindingDescriptions.emplace_back(binding, stride);

			return *this;
		}

		VertexLayout& bindAttribute(uint32_t location, uint32_t binding, vk::Format format, uint32_t offset)
		{
			mAttributeDescriptions.emplace_back(location, binding, format, offset);

			return *this;
		}

		[[nodiscard]] std::vector<vk::VertexInputBindingDescription> bindings() const { return mBindingDescriptions; }
		[[nodiscard]] std::vector <vk::VertexInputAttributeDescription> attributes() const { return mAttributeDescriptions; }

	private:

		std::vector<vk::VertexInputBindingDescription> mBindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> mAttributeDescriptions;

	};
}

namespace puffin::rendering
{
	inline util::VertexLayout VertexPC32::getLayoutVK()
	{
		return util::VertexLayout::begin()
			.bindInput(0, sizeof(VertexPC32))
			.bindAttribute(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPC32, pos))
			.bindAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPC32, color));
	}

	inline util::VertexLayout VertexPNC32::getLayoutVK()
	{
		return util::VertexLayout::begin()
			.bindInput(0, sizeof(VertexPNC32))
			.bindAttribute(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNC32, pos))
			.bindAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNC32, normal))
			.bindAttribute(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNC32, color));
	}

	inline util::VertexLayout VertexPNTV32::getLayoutVK()
	{
		return util::VertexLayout::begin()
			.bindInput(0, sizeof(VertexPNTV32))
			.bindAttribute(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNTV32, pos))
			.bindAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNTV32, normal))
			.bindAttribute(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexPNTV32, tangent))
			.bindAttribute(3, 0, vk::Format::eR32G32Sfloat, offsetof(VertexPNTV32, uv));
	}

	inline util::VertexLayout VertexP64NTV32::getLayoutVK()
	{
		return util::VertexLayout::begin()
			.bindInput(0, sizeof(VertexP64NTV32))
			.bindAttribute(0, 0, vk::Format::eR64G64B64Sfloat, offsetof(VertexP64NTV32, pos))
			.bindAttribute(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexP64NTV32, normal))
			.bindAttribute(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(VertexP64NTV32, tangent))
			.bindAttribute(3, 0, vk::Format::eR32G32Sfloat, offsetof(VertexP64NTV32, uv));
	}
}