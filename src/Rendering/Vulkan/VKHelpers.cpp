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

	AllocatedBuffer CreateBuffer(const vma::Allocator& allocator, size_t allocSize, vk::BufferUsageFlags usage,
		vma::MemoryUsage memoryUsage, vma::AllocationCreateFlags allocFlags, vk::MemoryPropertyFlags requiredFlags)
	{
		vk::BufferCreateInfo bufferInfo = { {}, allocSize, usage };

		vma::AllocationCreateInfo allocInfo = { allocFlags, memoryUsage, requiredFlags };

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
	         vma::MemoryUsage::eAutoPreferHost, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

		// Map vertex data to staging buffer
		void* data;
		VK_CHECK(renderer->GetAllocator().mapMemory(stagingBuffer.allocation, &data));
		memcpy(data, vertexData, verticesSize);
		renderer->GetAllocator().unmapMemory(stagingBuffer.allocation);

		// Allocate Vertex Buffer - Transfer Vertices into GPU Memory
		AllocatedBuffer vertexBuffer = CreateBuffer(renderer->GetAllocator(), verticesSize,
            { vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc }, 
            vma::MemoryUsage::eAutoPreferDevice);

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

		return vertexBuffer;
	}

	AllocatedBuffer InitIndexBuffer(std::shared_ptr<VKRenderSystem> renderer, const void* indexData,
		const size_t numIndices, const size_t indexSize)
	{
		uint32_t indicesSize = numIndices * indexSize;

		// Allocate Staging Buffer - Map Indices in CPU Memory
		AllocatedBuffer stagingBuffer = CreateBuffer(renderer->GetAllocator(), indicesSize, 
			{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferSrc }, 
			vma::MemoryUsage::eAutoPreferHost, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

		// Map Index data to staging buffer
		void* data;
		VK_CHECK(renderer->GetAllocator().mapMemory(stagingBuffer.allocation, &data));
		memcpy(data, indexData, indicesSize);
		renderer->GetAllocator().unmapMemory(stagingBuffer.allocation);

		// Allocate Index Buffer - Transfer indices into GPU memory
		AllocatedBuffer indexBuffer = CreateBuffer(renderer->GetAllocator(), indicesSize,
			{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

		// Copy from CPU memory to GPU memory
		ImmediateSubmit(renderer, [=](vk::CommandBuffer cmd)
		{
			vk::BufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = indicesSize;
			cmd.copyBuffer(stagingBuffer.buffer, indexBuffer.buffer, 1, &copy);
		});

		// Cleanup staging buffer, it is no longer needed
		renderer->GetAllocator().destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);

		return indexBuffer;
	}

	AllocatedImage CreateImage(std::shared_ptr<VKRenderSystem> renderer, vk::ImageCreateInfo imageInfo, vk::ImageViewCreateInfo imageViewInfo)
	{
		AllocatedImage allocImage;
		allocImage.format = imageInfo.format;

		// Create Image
		vma::AllocationCreateInfo imageAllocInfo = { {}, vma::MemoryUsage::eAutoPreferDevice, vk::MemoryPropertyFlagBits::eDeviceLocal };

		VK_CHECK(renderer->GetAllocator().createImage(&imageInfo, &imageAllocInfo, &allocImage.image, &allocImage.allocation, nullptr));

		// Create Image View
		imageViewInfo.image = allocImage.image;

		VK_CHECK(renderer->GetDevice().createImageView(&imageViewInfo, nullptr, &allocImage.imageView));

		return allocImage;
	}

	AllocatedImage InitDepthImage(std::shared_ptr<VKRenderSystem> renderer, vk::Extent3D extent, vk::Format format)
	{
		vk::ImageCreateInfo imageInfo = { {}, vk::ImageType::e2D, format, extent, 1, 1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment };

		vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 };

		vk::ImageViewCreateInfo imageViewInfo({}, {}, vk::ImageViewType::e2D, format, {}, subresourceRange);

		return CreateImage(renderer, imageInfo, imageViewInfo);
	}

	AllocatedImage InitTexture(std::shared_ptr<VKRenderSystem> renderer, const void* pixelData, uint32_t width, uint32_t height, uint32_t pixelSize, vk::Format format)
	{
		const vk::DeviceSize imageSize = width * height * pixelSize;

		// Allocate staging buffer on CPU for holding texture data to upload
		const AllocatedBuffer stagingBuffer = CreateBuffer(renderer->GetAllocator(), imageSize, 
			vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eAutoPreferHost, 
			vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

		// Copy texture data to buffer
		void* data;
		VK_CHECK(renderer->GetAllocator().mapMemory(stagingBuffer.allocation, &data));
		memcpy(data, pixelData, imageSize);
		renderer->GetAllocator().unmapMemory(stagingBuffer.allocation);

		// Allocate and create texture in GPU memory
		const vk::Extent3D imageExtent = { width, height, 1 };

		const vk::ImageCreateInfo imageInfo = { {}, vk::ImageType::e2D, format, imageExtent,
			1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eLinear,
			{ vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst } };

		const vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		const vk::ImageViewCreateInfo imageViewInfo = { {}, {}, vk::ImageViewType::e2D, format, {}, subresourceRange };

		const AllocatedImage texture = CreateImage(renderer, imageInfo, imageViewInfo);

		// Fill command for copying pixels to texture and transitioning image layout
		ImmediateSubmit(renderer, [=](vk::CommandBuffer cmd)
		{
			// Barrier image into transfer-receive layout
			const vk::ImageSubresourceRange range = { vk::ImageAspectFlagBits::eColor,
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
		renderer->GetAllocator().destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);

		return texture;
	}
}
