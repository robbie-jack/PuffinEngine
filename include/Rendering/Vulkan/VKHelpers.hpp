#pragma once

#include "VKTypes.hpp"

#include <functional>

namespace Puffin::Rendering::VK
{
	class VKRenderSystem;
}

namespace Puffin::Rendering::VK::Util
{
	static AllocatedBuffer CreateBuffer(const vma::Allocator& allocator, size_t allocSize, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage, vk::MemoryPropertyFlags requiredFlags = {});

	static AllocatedBuffer InitVertexBuffer(std::shared_ptr<VKRenderSystem> renderer, const void* vertexData, const size_t numVertices, const size_t vertexSize);

	static void ImmediateSubmit(std::shared_ptr<VKRenderSystem> renderSystem, std::function<void(VkCommandBuffer cmd)>&& function);
}
