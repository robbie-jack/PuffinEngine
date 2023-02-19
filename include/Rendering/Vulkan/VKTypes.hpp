#pragma once

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"

namespace Puffin::Rendering::VK
{
	struct UploadContext
	{
		vk::Fence uploadFence;
		vk::CommandPool commandPool;
		vk::CommandBuffer commandBuffer;
	};

	struct AllocatedBuffer
	{
		vk::Buffer buffer;
		vma::Allocation allocation;
	};
}