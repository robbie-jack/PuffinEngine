#pragma once

#include <functional>
#include <memory>

#include "puffin/rendering/vulkan/typesvk.h"

namespace puffin::rendering
{
	class RenderSubsystemVK;
}

namespace puffin::rendering::util
{
	void ImmediateSubmit(const RenderSubsystemVK* renderSystem, std::function<void(VkCommandBuffer cmd)>&& function);

	struct CopyDataBetweenBuffersParams
	{
		size_t dataSize;
		vk::Buffer srcBuffer;
		vk::Buffer dstBuffer;
		size_t srcOffset = 0;
		size_t dstOffset = 0;
	};

	void CopyDataBetweenBuffers(const RenderSubsystemVK* renderSystem, const CopyDataBetweenBuffersParams& params);

	struct CopyCPUDataIntoGPUBufferParams
	{
		AllocatedBuffer dstBuffer;
		size_t dataSize;
		void* srcData;
		size_t srcOffset = 0;
		size_t dstOffset = 0;
	};

	void CopyCPUDataIntoGPUBuffer(const RenderSubsystemVK* renderSystem, const CopyCPUDataIntoGPUBufferParams& params);

	struct CreateBufferParams
	{
		size_t allocSize;
		vk::BufferUsageFlags bufferUsage;
		vma::MemoryUsage memoryUsage = vma::MemoryUsage::eAuto;
		vma::AllocationCreateFlags allocFlags = {};
		vk::MemoryPropertyFlags requiredFlags = {};
	};

	AllocatedBuffer CreateBuffer(const vma::Allocator& allocator, const CreateBufferParams& params);

	struct CreateImageParams
	{
		vk::ImageCreateInfo imageInfo;
		vk::ImageViewCreateInfo imageViewInfo;
	};

	AllocatedImage CreateImage(const RenderSubsystemVK* renderSystem, const CreateImageParams& params);

	struct CreateFormattedImageParams
	{
		vk::Extent3D extent;
		vk::Format format;
	};

	AllocatedImage CreateColorImage(const RenderSubsystemVK* renderSystem, const CreateFormattedImageParams& params);
	AllocatedImage CreateDepthImage(const RenderSubsystemVK* renderSystem, const CreateFormattedImageParams& params);

	struct InitTextureParams
	{
		void* pixelData;
		size_t dataSize;
		uint32_t width;
		uint32_t height;
		vk::Format format;
	};
	AllocatedImage InitTexture(const RenderSubsystemVK* renderSystem, const InitTextureParams& params);
}