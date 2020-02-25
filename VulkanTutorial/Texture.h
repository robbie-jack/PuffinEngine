#pragma once

#include <vulkan/vulkan.h>

class Texture
{
public:

	void Cleanup(VkDevice device);

	inline VkImage& GetTextureImage() { return textureImage; };
	inline VkDeviceMemory& GetTextureMemory() { return textureImageMemory; };
	inline VkImageView& GetImageView() { return textureImageView; };

private:
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
};