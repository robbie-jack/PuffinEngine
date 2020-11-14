#pragma once

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
	}
}