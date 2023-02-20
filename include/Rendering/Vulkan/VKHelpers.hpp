#pragma once

#include "VKTypes.hpp"

#include <functional>

namespace Puffin::Rendering::VK
{
	class VKRenderSystem;
}

namespace Puffin::Rendering::VK::Util
{
	AllocatedBuffer InitVertexBuffer(std::shared_ptr<VKRenderSystem> renderer, const void* vertexData, const size_t numVertices, const size_t vertexSize);

	AllocatedBuffer InitIndexBuffer(std::shared_ptr<VKRenderSystem> renderer, const void* indexData, const size_t numIndices, const size_t indexSize);

	AllocatedBuffer CreateBuffer(const vma::Allocator& allocator, size_t allocSize, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage, vma::
		AllocationCreateFlags allocFlags = {}, vk::MemoryPropertyFlags requiredFlags = {});

	void ImmediateSubmit(std::shared_ptr<VKRenderSystem> renderSystem, std::function<void(VkCommandBuffer cmd)>&& function);
}
