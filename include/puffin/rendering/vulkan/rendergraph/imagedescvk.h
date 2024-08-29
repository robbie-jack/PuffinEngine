#pragma once

#include <cstdint>

#include "vulkan/vulkan.hpp"

namespace puffin::rendering
{
	/*
	 * Enum for defining what size an attachment should be created with
	 */
	enum class ImageSizeVK
	{
		Absolute = 0,			// Size should be set to absolute value of attachment description
		SwapchainRelative,		// Size should be relative to the size of the swapchain
		RenderExtentRelative,	// Size should be relative to size of defined render extent
	};

	struct ImageDescVK
	{
		ImageSizeVK imageSize = ImageSizeVK::RenderExtentRelative;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 1;
		float widthMult = 1.0f;		// Multiplier for width if using a relative size type
		float heightMult = 1.0f;	// Multiplier for height if using a relative size type
		float depthMult = 1.0f;		// Multiplier for depth if using a relative size type
		vk::Format format = vk::Format::eUndefined;
		vk::ImageUsageFlags usageFlags;
		vk::ImageTiling tiling = vk::ImageTiling::eOptimal; // Linear for Textures, Optimal for everything else;
		vk::ImageType type = vk::ImageType::e2D;
		uint8_t samples = 1;
		uint8_t mipLevels = 1;
		uint8_t layers = 1;
		bool persistent = false; // Whether this image should persist between frames or not
	};
}
