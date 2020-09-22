#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

// Image, View and Allocation attached to a framebuffer
struct FrameBufferAttachment
{
	VkImage image;
	VmaAllocation allocation;
	VkImageView imageView;
};