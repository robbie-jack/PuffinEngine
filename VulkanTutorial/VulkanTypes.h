#pragma once

#ifndef VULKAN_TYPES_H
#define VULKAN_TYPES_H

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

namespace Puffin
{
	namespace Rendering
	{
		// Image, View and Allocation attached to a framebuffer
		struct FrameBufferAttachment
		{
			VkImage image;
			VmaAllocation allocation;
			VkImageView imageView;
		};

		typedef FrameBufferAttachment Texture;

		struct AllocatedBuffer
		{
			VkBuffer buffer;
			VmaAllocation allocation;
		};
	}
}

#endif // VULKAN_TYPES_H