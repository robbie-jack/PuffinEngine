#pragma once

#ifndef VULKAN_TYPES_H
#define VULKAN_TYPES_H

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

namespace Puffin
{
	namespace Rendering
	{
		// Allocated Image/View
		struct AllocatedImage
		{
			VkImage image;
			VmaAllocation allocation;
			VkImageView imageView;
		};

		typedef AllocatedImage Texture;

		struct AllocatedBuffer
		{
			VkBuffer buffer;
			VmaAllocation allocation;
		};

		struct Material
		{
			VkDescriptorSet textureSet;
			VkPipeline pipeline;
			VkPipelineLayout pipelineLayout;
		};
	}
}

#endif // VULKAN_TYPES_H