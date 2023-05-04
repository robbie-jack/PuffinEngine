#pragma once

#include "VKTypes.hpp"

#include <functional>

namespace puffin::Rendering::VK
{
	class VKRenderSystem;
}

namespace puffin::Rendering::VK::Util
{
	void ImmediateSubmit(std::shared_ptr<VKRenderSystem> renderSystem, std::function<void(VkCommandBuffer cmd)>&& function);

	void CopyDataBetweenBuffers(std::shared_ptr<VKRenderSystem> renderer, vk::Buffer srcBuffer, vk::Buffer dstBuffer, 
		uint32_t dataSize, uint32_t srcOffset = 0, uint32_t dstOffset = 0);

	void LoadCPUDataIntoGPUBuffer(std::shared_ptr<VKRenderSystem> renderer, vk::BufferUsageFlags usageFlags, AllocatedBuffer dstBuffer, uint32_t dataSize, 
		const void* data, uint32_t srcOffset = 0, uint32_t dstOffset = 0);

	AllocatedBuffer CreateBuffer(const vma::Allocator& allocator, size_t allocSize, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage, vma::
		AllocationCreateFlags allocFlags = {}, vk::MemoryPropertyFlags requiredFlags = {});

	AllocatedBuffer InitVertexBuffer(std::shared_ptr<VKRenderSystem> renderer, const void* vertexData, const size_t numVertices, const size_t vertexSize);

	AllocatedBuffer InitIndexBuffer(std::shared_ptr<VKRenderSystem> renderer, const void* indexData, const size_t numIndices, const size_t indexSize);

	AllocatedImage CreateImage(std::shared_ptr<VKRenderSystem> renderer, vk::ImageCreateInfo imageInfo, vk::ImageViewCreateInfo imageViewInfo);

	AllocatedImage InitDepthImage(std::shared_ptr<VKRenderSystem> renderer, vk::Extent3D extent, vk::Format format);

	AllocatedImage InitTexture(std::shared_ptr<VKRenderSystem> renderer, const void* pixelData, uint32_t width, uint32_t height, uint32_t pixelSize, vk::Format format);
}