#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

namespace Puffin
{
	namespace Rendering
	{
		class Texture
		{
		public:

			void Cleanup(VkDevice device);

			inline VkImage& GetTextureImage() { return textureImage; };
			inline VmaAllocation& GetTextureAllocation() { return textureAllocation; };
			inline VkImageView& GetImageView() { return textureImageView; };

		private:
			VkImage textureImage;
			VmaAllocation textureAllocation;
			VkImageView textureImageView;
		};
	}
}