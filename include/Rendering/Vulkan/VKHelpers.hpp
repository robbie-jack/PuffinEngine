#pragma once

#include "VKTypes.hpp"

#include <functional>

namespace Puffin::Rendering::VK
{
	class VKRenderSystem;
}

namespace Puffin::Rendering::VK::Util
{
	void ImmediateSubmit(std::shared_ptr<VKRenderSystem> renderSystem, std::function<void(VkCommandBuffer cmd)>&& function);

	AllocatedBuffer CreateBuffer(const vma::Allocator& allocator, size_t allocSize, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage, vma::
		AllocationCreateFlags allocFlags = {}, vk::MemoryPropertyFlags requiredFlags = {});

	AllocatedBuffer InitVertexBuffer(std::shared_ptr<VKRenderSystem> renderer, const void* vertexData, const size_t numVertices, const size_t vertexSize);

	AllocatedBuffer InitIndexBuffer(std::shared_ptr<VKRenderSystem> renderer, const void* indexData, const size_t numIndices, const size_t indexSize);

	AllocatedImage InitImage(std::shared_ptr<VKRenderSystem> renderer, vk::ImageCreateInfo imageInfo, vk::ImageViewCreateInfo imageViewInfo);

	AllocatedImage InitDepthImage(std::shared_ptr<VKRenderSystem> renderer, vk::Extent3D extent, vk::Format format);
}