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

	enum class AttachmentTypeVK
	{
		Color,
		Depth
	};

	struct AttachmentDescVK
	{
		ImageSizeVK imageSize = ImageSizeVK::RenderExtentRelative;
		AttachmentTypeVK type = AttachmentTypeVK::Color;
		vk::Format format = vk::Format::eUndefined;
		uint32_t width = 0;
		uint32_t height = 0;
		float widthMult = 1.0f;
		float heightMult = 1.0f;
	};

	struct ImageDescVK
	{
		vk::ImageCreateInfo info;
		vk::ImageViewCreateInfo viewInfo;
		bool persistent = false;
	};

	struct BufferDescVK
	{
		vk::DeviceSize size = 0;
		vk::BufferUsageFlags usage = {};
		vma::MemoryUsage memoryUsage = vma::MemoryUsage::eAuto;
		vma::AllocationCreateFlags allocFlags = {};
		vk::MemoryPropertyFlags propFlags = {};
		bool persistent = false; // Whether this buffer should persist between frames or not
	};

	struct DescriptorLayoutBindingVK
	{
		vk::DescriptorType type;
		uint32_t count = 1;
		vk::ShaderStageFlags stageFlags = {};
		vk::DescriptorBindingFlags bindingFlags = {};
		vk::Sampler* sampler = nullptr;
	};

	struct DescriptorLayoutDescVK
	{
		std::vector<DescriptorLayoutBindingVK> bindings;
	};
}