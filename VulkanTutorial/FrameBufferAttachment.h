#pragma once

#ifndef FRAME_BUFFER_ATTACHMENT_H
#define FRAME_BUFFER_ATTACHMENT_H

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

#endif // FRAME_BUFFER_ATTACHMENT_H