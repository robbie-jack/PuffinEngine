#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include "FrameBufferAttachment.h"

namespace Puffin
{
	namespace Rendering
	{
		class Texture
		{
		public:

			//void Cleanup(VkDevice device, VmaAllocator& allocator);

			inline FrameBufferAttachment& GetTextureAttachment() { return textureAttachment; };
			inline VkImage& GetTextureImage() { return textureAttachment.image; };
			inline VmaAllocation& GetTextureAllocation() { return textureAttachment.allocation; };
			inline VkImageView& GetImageView() { return textureAttachment.imageView; };

		private:
			FrameBufferAttachment textureAttachment;
			/*VkImage textureImage;
			VmaAllocation textureAllocation;
			VkImageView textureImageView;*/
		};
	}
}