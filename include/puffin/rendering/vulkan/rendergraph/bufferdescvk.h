#pragma once

#include <vulkan/vulkan.hpp>

namespace puffin::rendering
{
	struct BufferDescVK
	{
		vk::DeviceSize size = 0;
		vk::BufferUsageFlags usageFlags;
	};
}
