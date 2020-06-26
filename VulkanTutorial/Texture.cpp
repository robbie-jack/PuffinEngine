#include "Texture.h"

namespace Puffin
{
	namespace Rendering
	{
		void Texture::Cleanup(VkDevice device)
		{
			vkDestroyImage(device, textureImage, nullptr);
			vkFreeMemory(device, textureImageMemory, nullptr);
			vkDestroyImageView(device, textureImageView, nullptr);
		}
	}
}