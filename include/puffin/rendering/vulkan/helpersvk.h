#pragma once

#include <functional>
#include <memory>

#include "puffin/rendering/vulkan/typesvk.h"

namespace puffin::rendering
{
	class RenderSubystemVK;
}

namespace puffin::rendering::util
{
	void ImmediateSubmit(const RenderSubystemVK* renderSystem, std::function<void(VkCommandBuffer cmd)>&& function);

	struct CopyDataBetweenBuffersParams
	{
		size_t dataSize;
		vk::Buffer srcBuffer;
		vk::Buffer dstBuffer;
		size_t srcOffset = 0;
		size_t dstOffset = 0;
	};

	void CopyDataBetweenBuffers(const RenderSubystemVK* renderSystem, const CopyDataBetweenBuffersParams& params);

	struct CopyCPUDataIntoGPUBufferParams
	{
		AllocatedBuffer dstBuffer;
		size_t dataSize;
		void* srcData;
		size_t srcOffset = 0;
		size_t dstOffset = 0;
	};

	void CopyCPUDataIntoGPUBuffer(const RenderSubystemVK* renderSystem, const CopyCPUDataIntoGPUBufferParams& params);

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

	AllocatedImage CreateImage(const RenderSubystemVK* renderSystem, const CreateImageParams& params);

	struct CreateFormattedImageParams
	{
		vk::Extent3D extent;
		vk::Format format;
	};

	AllocatedImage CreateColorImage(const RenderSubystemVK* renderSystem, const CreateFormattedImageParams& params);
	AllocatedImage CreateDepthImage(const RenderSubystemVK* renderSystem, const CreateFormattedImageParams& params);

	struct InitTextureParams
	{
		void* pixelData;
		size_t dataSize;
		uint32_t width;
		uint32_t height;
		vk::Format format;
	};
	AllocatedImage InitTexture(const RenderSubystemVK* renderSystem, const InitTextureParams& params);
}