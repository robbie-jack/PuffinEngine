#pragma once

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"

namespace Puffin::Rendering::VK
{
	struct AllocatedBuffer
	{
		vk::Buffer buffer;
		vma::Allocation allocation;
	};
}