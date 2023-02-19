#include "Rendering/Vulkan/VKHelpers.hpp"

#include "Rendering/Vulkan/VKRenderSystem.hpp"

#include <iostream>

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		vk::Result err = x;                                         \
		if (err != vk::Result::eSuccess)                            \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

namespace Puffin::Rendering::VK::Util
{

	AllocatedBuffer CreateBuffer(const vma::Allocator& allocator, size_t allocSize, vk::BufferUsageFlags usage,
		vma::MemoryUsage memoryUsage, vk::MemoryPropertyFlags requiredFlags)
	{
		vk::BufferCreateInfo bufferInfo = { {}, allocSize, usage };

		vma::AllocationCreateInfo allocInfo = { {}, memoryUsage, requiredFlags };

		AllocatedBuffer buffer;

		VK_CHECK(allocator.createBuffer(&bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, nullptr));

		return buffer;
	}

	AllocatedBuffer InitVertexBuffer(std::shared_ptr<VKRenderSystem> renderer, const void* vertexData, const size_t numVertices, const size_t vertexSize)
	{
		// Copy Loaded Mesh data into mesh vertex buffer
		uint32_t verticesSize = numVertices * vertexSize;

		// Allocate Staging Buffer - Map Vertices in CPU Memory
		AllocatedBuffer stagingBuffer = CreateBuffer(renderer->GetAllocator(), verticesSize,
			{ vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc }, 
				vma::MemoryUsage::eCpuOnly);

		// Map vertex data to staging buffer
		void* data;
		VK_CHECK(renderer->GetAllocator().mapMemory(stagingBuffer.allocation, &data));
		memcpy(data, vertexData, verticesSize);
		renderer->GetAllocator().unmapMemory(stagingBuffer.allocation);

		// Allocate Vertex Buffer - Transfer Vertices into GPU Memory
		AllocatedBuffer vertexBuffer = CreateBuffer(renderer->GetAllocator(), verticesSize,
			{ vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc }, 
				vma::MemoryUsage::eGpuOnly);

		// Copy from CPU Memory to GPU Memory
		ImmediateSubmit(renderer, [=](vk::CommandBuffer cmd)
		{
			vk::BufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = verticesSize;
			cmd.copyBuffer(stagingBuffer.buffer, vertexBuffer.buffer, 1, &copy);
		});

		// Cleanup Staging Buffer Immediately, It is no longer needed
		renderer->GetAllocator().destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);
	}

	void ImmediateSubmit(std::shared_ptr<VKRenderSystem> renderSystem, std::function<void(VkCommandBuffer cmd)>&& function)
	{
		vk::CommandBuffer cmd = renderSystem->GetUploadContext().commandBuffer;

		vk::CommandBufferBeginInfo cmdBeginInfo = { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
		VK_CHECK(cmd.begin(&cmdBeginInfo));

		function(cmd);

		cmd.end();

		vk::SubmitInfo submit = { {}, {}, {}, 1, &cmd };

		VK_CHECK(renderSystem->GetGraphicsQueue().submit(1, &submit, renderSystem->GetUploadContext().uploadFence));

		VK_CHECK(renderSystem->GetDevice().waitForFences(1, &renderSystem->GetUploadContext().uploadFence, true, 9999999999));
		VK_CHECK(renderSystem->GetDevice().resetFences(1, &renderSystem->GetUploadContext().uploadFence));

		renderSystem->GetDevice().resetCommandPool(renderSystem->GetUploadContext().commandPool);
	}
}
