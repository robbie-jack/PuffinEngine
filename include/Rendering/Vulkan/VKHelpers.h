#pragma once

#include "VKTypes.h"

#include <functional>

namespace puffin::rendering
{
	class VKRenderSystem;
}

namespace puffin::rendering::util
{
	void immediateSubmit(const std::shared_ptr<VKRenderSystem>& renderSystem, std::function<void(VkCommandBuffer cmd)>&& function);

	void copyDataBetweenBuffers(const std::shared_ptr<VKRenderSystem>& renderer, vk::Buffer srcBuffer, vk::Buffer dstBuffer, 
	                            uint32_t dataSize, uint32_t srcOffset = 0, uint32_t dstOffset = 0);

	void loadCpuDataIntoGPUBuffer(const std::shared_ptr<VKRenderSystem>& renderer, vk::BufferUsageFlags usageFlags, const AllocatedBuffer& dstBuffer, uint32_t dataSize, 
	                              const void* data, uint32_t srcOffset = 0, uint32_t dstOffset = 0);

	AllocatedBuffer createBuffer(const vma::Allocator& allocator, size_t allocSize, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage, vma::
		AllocationCreateFlags allocFlags = {}, vk::MemoryPropertyFlags requiredFlags = {});

	AllocatedBuffer initVertexBuffer(const std::shared_ptr<VKRenderSystem>& renderer, const void* vertexData, const size_t numVertices, const size_t vertexSize);

	AllocatedBuffer initIndexBuffer(const std::shared_ptr<VKRenderSystem>& renderer, const void* indexData, const size_t numIndices, const size_t indexSize);

	AllocatedImage createImage(const std::shared_ptr<VKRenderSystem>& renderer, const vk::ImageCreateInfo& imageInfo, vk::ImageViewCreateInfo imageViewInfo);

	AllocatedImage initDepthImage(const std::shared_ptr<VKRenderSystem>& renderer, vk::Extent3D extent, vk::Format format);

	AllocatedImage initTexture(const std::shared_ptr<VKRenderSystem>& renderer, const void* pixelData, uint32_t width, uint32_t height, uint32_t pixelSize, vk::Format format);
}