#include "Rendering/Vulkan/VKHelpers.h"

#include "Rendering/Vulkan/VKRenderSystem.h"

#include <iostream>
#include <algorithm>

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

namespace puffin::rendering::util
{
	void immediateSubmit(const std::shared_ptr<VKRenderSystem>& renderSystem,
	                     std::function<void(VkCommandBuffer cmd)>&& function)
	{
		vk::CommandBuffer cmd = renderSystem->uploadContext().commandBuffer;

		const vk::CommandBufferBeginInfo cmdBeginInfo = { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
		VK_CHECK(cmd.begin(&cmdBeginInfo));

		function(cmd);

		cmd.end();

		const vk::SubmitInfo submit = { {}, {}, {}, 1, &cmd };

		VK_CHECK(renderSystem->graphicsQueue().submit(1, &submit, renderSystem->uploadContext().uploadFence));

		VK_CHECK(renderSystem->device().waitForFences(1, &renderSystem->uploadContext().uploadFence, true, 9999999999));
		VK_CHECK(renderSystem->device().resetFences(1, &renderSystem->uploadContext().uploadFence));

		renderSystem->device().resetCommandPool(renderSystem->uploadContext().commandPool);
	}

	void copyDataBetweenBuffers(const std::shared_ptr<VKRenderSystem>& renderer, const vk::Buffer srcBuffer,
	                            const vk::Buffer dstBuffer,
	                            const uint32_t dataSize, const uint32_t srcOffset, const uint32_t dstOffset)
	{
		immediateSubmit(renderer, [=](const vk::CommandBuffer cmd)
		{
			vk::BufferCopy copy;
			copy.dstOffset = dstOffset;
			copy.srcOffset = srcOffset;
			copy.size = dataSize;
			cmd.copyBuffer(srcBuffer, dstBuffer, 1, &copy);
		});
	}

	AllocatedBuffer createBuffer(const vma::Allocator& allocator, const size_t allocSize, const vk::BufferUsageFlags usage,
	                             const vma::MemoryUsage memoryUsage, const vma::AllocationCreateFlags allocFlags, const vk::MemoryPropertyFlags requiredFlags)
	{
		const vk::BufferCreateInfo bufferInfo = { {}, allocSize, usage };

		const vma::AllocationCreateInfo allocInfo = { allocFlags, memoryUsage, requiredFlags };

		AllocatedBuffer buffer;

		VK_CHECK(allocator.createBuffer(&bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocInfo));

		return buffer;
	}

	void copyCPUDataIntoGPUBuffer(const std::shared_ptr<VKRenderSystem>& renderer, const AllocatedBuffer& dstBuffer, const uint32_t dataSize,
	                              const void* data, const uint32_t srcOffset, const uint32_t dstOffset)
	{
		// If rebar is enabled and buffer is in host visible memory, copy directly to buffer
		if (const vk::MemoryPropertyFlags memPropFlags = renderer->allocator().getAllocationMemoryProperties(dstBuffer.allocation); renderer->isReBarEnabled() 
			&& (memPropFlags & vk::MemoryPropertyFlagBits::eHostVisible))
		{
			const auto* dataChar = static_cast<const char*>(data);

			std::copy_n(dataChar, dataSize, static_cast<char*>(dstBuffer.allocInfo.pMappedData) + dstOffset);
		}
		// If rebar is not enabled or buffer is not in host visible, copy data via staging buffer
		else
		{
			// Allocate Staging Buffer - Map Vertices in CPU Memory
			const AllocatedBuffer stagingBuffer = createBuffer(renderer->allocator(), dataSize,
			                                                   vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eAuto, 
			                                                   { vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped });

			const auto* dataChar = static_cast<const char*>(data);
			auto* mappedDataChar = static_cast<char*>(stagingBuffer.allocInfo.pMappedData);

			std::copy_n(dataChar, dataSize, mappedDataChar);

			// Copy from CPU Memory to GPU Memory
			copyDataBetweenBuffers(renderer, stagingBuffer.buffer, dstBuffer.buffer, dataSize, srcOffset, dstOffset);

			// Cleanup Staging Buffer Immediately, It is no longer needed
			renderer->allocator().destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);
		}
	}

	AllocatedBuffer initVertexBuffer(const std::shared_ptr<VKRenderSystem>& renderer, const void* vertexData, const size_t numVertices, const size_t vertexSize)
	{
		// Copy Loaded Mesh data into mesh vertex buffer
		const uint32_t vertexBufferSize = numVertices * vertexSize;

		// Allocate Vertex Buffer - Transfer Vertices into GPU Memory
		AllocatedBuffer vertexBuffer = createBuffer(renderer->allocator(), vertexBufferSize,
            { vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc }, 
			vma::MemoryUsage::eAuto, { vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead });

		copyCPUDataIntoGPUBuffer(renderer, vertexBuffer, vertexBufferSize, vertexData);

		return vertexBuffer;
	}

	AllocatedBuffer initIndexBuffer(const std::shared_ptr<VKRenderSystem>& renderer, const void* indexData,
	                                const size_t numIndices, const size_t indexSize)
	{
		const uint32_t indexBufferSize = numIndices * indexSize;

		// Allocate Index Buffer - Transfer indices into GPU memory
		AllocatedBuffer indexBuffer = createBuffer(renderer->allocator(), indexBufferSize,
			{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAuto, { vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead });

		copyCPUDataIntoGPUBuffer(renderer, indexBuffer, indexBufferSize, indexData);

		return indexBuffer;
	}

	AllocatedImage createImage(const std::shared_ptr<VKRenderSystem>& renderer, const vk::ImageCreateInfo& imageInfo, vk::ImageViewCreateInfo imageViewInfo)
	{
		AllocatedImage allocImage;
		allocImage.format = imageInfo.format;

		// Create Image
		constexpr vma::AllocationCreateInfo imageAllocInfo = { {}, vma::MemoryUsage::eAutoPreferDevice, vk::MemoryPropertyFlagBits::eDeviceLocal };

		VK_CHECK(renderer->allocator().createImage(&imageInfo, &imageAllocInfo, &allocImage.image, &allocImage.allocation, &allocImage.allocInfo));

		// Create Image View
		imageViewInfo.image = allocImage.image;

		VK_CHECK(renderer->device().createImageView(&imageViewInfo, nullptr, &allocImage.imageView));

		return allocImage;
	}

	AllocatedImage initDepthImage(const std::shared_ptr<VKRenderSystem>& renderer, const vk::Extent3D extent, const vk::Format format)
	{
		const vk::ImageCreateInfo imageInfo = { {}, vk::ImageType::e2D, format, extent, 1, 1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment };

		constexpr vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 };

		const vk::ImageViewCreateInfo imageViewInfo({}, {}, vk::ImageViewType::e2D, format, {}, subresourceRange);

		return createImage(renderer, imageInfo, imageViewInfo);
	}

	AllocatedImage initTexture(const std::shared_ptr<VKRenderSystem>& renderer, const void* pixelData, const uint32_t width, const uint32_t height, const uint32_t pixelSize, const vk::Format format)
	{
		const vk::DeviceSize imageSize = height * width * pixelSize;

		// Allocate staging buffer on CPU for holding texture data to upload
		const AllocatedBuffer stagingBuffer = createBuffer(renderer->allocator(), imageSize, 
			vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eAuto, 
			vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

		// Copy texture data to buffer
		void* data;
		VK_CHECK(renderer->allocator().mapMemory(stagingBuffer.allocation, &data));
		memcpy(data, pixelData, imageSize);
		renderer->allocator().unmapMemory(stagingBuffer.allocation);

		// Allocate and create texture in GPU memory
		const vk::Extent3D imageExtent = { width, height, 1 };

		const vk::ImageCreateInfo imageInfo = { {}, vk::ImageType::e2D, format, imageExtent,
			1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eLinear,
			{ vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst } };

		constexpr vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		const vk::ImageViewCreateInfo imageViewInfo = { {}, {}, vk::ImageViewType::e2D, format, {}, subresourceRange };

		const AllocatedImage texture = createImage(renderer, imageInfo, imageViewInfo);

		// Fill command for copying pixels to texture and transitioning image layout
		immediateSubmit(renderer, [=](vk::CommandBuffer cmd)
		{
			// Barrier image into transfer-receive layout
			constexpr vk::ImageSubresourceRange range = { vk::ImageAspectFlagBits::eColor,
				0, 1, 0, 1 };

			const vk::ImageMemoryBarrier imageBarrierToTransfer = { {}, vk::AccessFlagBits::eTransferWrite,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
				0, 0, texture.image, range };

			cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {},
				0, nullptr, 0, nullptr, 1, & imageBarrierToTransfer);

			// Copy buffer into image
			const vk::BufferImageCopy copyRegion = { 0, 0, 0,
				{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, {}, imageExtent };

			cmd.copyBufferToImage(stagingBuffer.buffer, texture.image, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);

			// Barrier image into shader readable layout
			vk::ImageMemoryBarrier imageBarrierToReadable = imageBarrierToTransfer;
			imageBarrierToReadable.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			imageBarrierToReadable.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imageBarrierToReadable.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			imageBarrierToReadable.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {},
				0, nullptr, 0, nullptr, 1, & imageBarrierToReadable);
		});

		// Cleanup Staging Buffer Immediately, It is no longer needed
		renderer->allocator().destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);

		return texture;
	}
}
