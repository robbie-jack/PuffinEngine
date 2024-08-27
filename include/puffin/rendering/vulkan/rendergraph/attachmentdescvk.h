#pragma once

#include <cstdint>

#include "vulkan/vulkan.hpp"

namespace puffin::rendering
{
	/*
	 * Enum for defining what size an attachment should be created with
	 */
	enum class AttachmentSizeTypeVK
	{
		Absolute = 0,			// Size should be set to absolute value of attachment description
		SwapchainRelative,		// Size should be relative to the size of the swapchain
		RenderExtentRelative,	// Size should be relative to size of defined render extent
	};

	struct AttachmentDescVK
	{
		AttachmentSizeTypeVK attachmentSizeType = AttachmentSizeTypeVK::RenderExtentRelative;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
		float widthMult = 1.0f;		// Multiplier for width if using a relative size type
		float heightMult = 1.0f;	// Multiplier for height if using a relative size type
		float depthMult = 1.0f;		// Multiplier for depth if using a relative size type
		vk::Format format = vk::Format::eUndefined;
		vk::ImageUsageFlags usageFlags;
	};
}
