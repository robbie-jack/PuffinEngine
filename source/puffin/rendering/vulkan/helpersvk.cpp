#include "puffin/rendering/vulkan/helpersvk.h"

#include <iostream>
#include <algorithm>

#include "puffin/rendering/vulkan/rendersubsystemvk.h"

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
	void ImmediateSubmit(const RenderSubsystemVK* renderSystem,
	                     std::function<void(VkCommandBuffer cmd)>&& function)
	{
		vk::CommandBuffer cmd = renderSystem->GetUploadContext().commandBuffer;

		const vk::CommandBufferBeginInfo cmdBeginInfo = { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
		VK_CHECK(cmd.begin(&cmdBeginInfo));

		function(cmd);

		cmd.end();

		const vk::SubmitInfo submit = { {}, {}, {}, 1, &cmd };

		VK_CHECK(renderSystem->GetGraphicsQueue().submit(1, &submit, renderSystem->GetUploadContext().uploadFence));

		VK_CHECK(renderSystem->GetDevice().waitForFences(1, &renderSystem->GetUploadContext().uploadFence, true, 9999999999));
		VK_CHECK(renderSystem->GetDevice().resetFences(1, &renderSystem->GetUploadContext().uploadFence));

		renderSystem->GetDevice().resetCommandPool(renderSystem->GetUploadContext().commandPool);
	}

	void CopyDataBetweenBuffers(const RenderSubsystemVK* renderSystem, const CopyDataBetweenBuffersParams& params)
	{
		ImmediateSubmit(renderSystem, [=](const vk::CommandBuffer cmd)
		{
			vk::BufferCopy copy;
			copy.dstOffset = params.dstOffset;
			copy.srcOffset = params.srcOffset;
			copy.size = params.dataSize;
			cmd.copyBuffer(params.srcBuffer, params.dstBuffer, 1, &copy);
		});
	}

	AllocatedBuffer CreateBuffer(const vma::Allocator& allocator,
	                             const
	                             CreateBufferParams& params)
	{
		const vk::BufferCreateInfo bufferInfo = { {}, params.allocSize, params.bufferUsage };

		const vma::AllocationCreateInfo allocInfo = { params.allocFlags, params.memoryUsage, params.requiredFlags };

		AllocatedBuffer buffer;

		VK_CHECK(allocator.createBuffer(&bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocInfo));

		return buffer;
	}

	void CopyCPUDataIntoGPUBuffer(const RenderSubsystemVK* renderSystem,
	                              const CopyCPUDataIntoGPUBufferParams& params)
	{
		// If rebar is enabled and buffer is in host visible memory, copy directly to buffer
		if (const vk::MemoryPropertyFlags memPropFlags = renderSystem->GetAllocator().getAllocationMemoryProperties(params.dstBuffer.allocation); renderSystem->GetRebarEnabled() 
			&& (memPropFlags & vk::MemoryPropertyFlagBits::eHostVisible))
		{
			const auto* dataChar = static_cast<const char*>(params.srcData);

			std::copy_n(dataChar, params.dataSize, static_cast<char*>(params.dstBuffer.allocInfo.pMappedData) + params.dstOffset);
		}
		// If rebar is not enabled or buffer is not in host visible, copy data via staging buffer
		else
		{
			// Allocate Staging Buffer - Map data in CPU Memory
			CreateBufferParams createParams;
			createParams.allocSize = params.dataSize;
			createParams.bufferUsage = vk::BufferUsageFlagBits::eTransferSrc;
			createParams.allocFlags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped;
			const AllocatedBuffer stagingBuffer = CreateBuffer(renderSystem->GetAllocator(), createParams);

			const auto* dataChar = static_cast<const char*>(params.srcData);
			auto* mappedDataChar = static_cast<char*>(stagingBuffer.allocInfo.pMappedData);

			std::copy_n(dataChar, params.dataSize, mappedDataChar);

			// Copy from CPU Memory to GPU Memory
			CopyDataBetweenBuffersParams copyParams;
			copyParams.srcBuffer = stagingBuffer.buffer;
			copyParams.dstBuffer = params.dstBuffer.buffer;
			copyParams.dataSize = params.dataSize;
			copyParams.srcOffset = params.srcOffset;
			copyParams.dstOffset = params.dstOffset;
			CopyDataBetweenBuffers(renderSystem, copyParams);

			// Cleanup Staging Buffer Immediately, It is no longer needed
			renderSystem->GetAllocator().destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);
		}
	}

	AllocatedImage CreateImage(const RenderSubsystemVK* renderSystem, const CreateImageParams& params)
	{
		AllocatedImage allocImage;
		allocImage.format = params.imageInfo.format;

		// Create Image
		constexpr vma::AllocationCreateInfo imageAllocInfo = { {}, vma::MemoryUsage::eAutoPreferDevice, vk::MemoryPropertyFlagBits::eDeviceLocal };

		VK_CHECK(renderSystem->GetAllocator().createImage(&params.imageInfo, &imageAllocInfo, &allocImage.image, &allocImage.allocation, &allocImage.allocInfo));

		// Create Image View
		vk::ImageViewCreateInfo imageViewCreateInfo = params.imageViewInfo;
		imageViewCreateInfo.image = allocImage.image;

		VK_CHECK(renderSystem->GetDevice().createImageView(&imageViewCreateInfo, nullptr, &allocImage.imageView));

		return allocImage;
	}

	AllocatedImage CreateColorImage(const RenderSubsystemVK* renderSystem, const CreateFormattedImageParams& params)
	{
		CreateImageParams createImageParams;

		createImageParams.imageInfo = { {}, vk::ImageType::e2D, params.format, params.extent, 1, 1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, {vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment } };

		constexpr vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		createImageParams.imageViewInfo = { {}, {}, vk::ImageViewType::e2D, params.format, {}, subresourceRange };

		return CreateImage(renderSystem, createImageParams);
	}

	AllocatedImage CreateDepthImage(const RenderSubsystemVK* renderSystem, const CreateFormattedImageParams& params)
	{
		CreateImageParams createImageParams;

		createImageParams.imageInfo = { {}, vk::ImageType::e2D, params.format, params.extent, 1, 1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, { vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eDepthStencilAttachment } };

		constexpr vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 };

		createImageParams.imageViewInfo = { {}, {}, vk::ImageViewType::e2D, params.format, {}, subresourceRange };

		return CreateImage(renderSystem, createImageParams);
	}

	AllocatedImage InitTexture(const RenderSubsystemVK* renderSystem, const InitTextureParams& params)
	{
		// Allocate staging buffer on CPU for holding texture data to upload
		CreateBufferParams createBufferParams;
		createBufferParams.allocSize = params.dataSize;
		createBufferParams.bufferUsage = vk::BufferUsageFlagBits::eTransferSrc;
		createBufferParams.allocFlags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped;
		const AllocatedBuffer stagingBuffer = CreateBuffer(renderSystem->GetAllocator(), createBufferParams);

		// Copy texture data to buffer
		const auto* dataChar = static_cast<const char*>(params.pixelData);
		auto* mappedDataChar = static_cast<char*>(stagingBuffer.allocInfo.pMappedData);

		std::copy_n(dataChar, params.dataSize, mappedDataChar);

		// Allocate and create texture in GPU memory
		const vk::Extent3D imageExtent = { params.width, params.height, 1 };

		CreateImageParams createImageParams;
		createImageParams.imageInfo = { {}, vk::ImageType::e2D, params.format, imageExtent,
			1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eLinear,
			{ vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst } };

		constexpr vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		createImageParams.imageViewInfo = { {}, {}, vk::ImageViewType::e2D, params.format, {}, subresourceRange };

		const AllocatedImage texture = CreateImage(renderSystem, createImageParams);

		// Fill command for copying pixels to texture and transitioning image layout
		ImmediateSubmit(renderSystem, [=](vk::CommandBuffer cmd)
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
		renderSystem->GetAllocator().destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);

		return texture;
	}

	vk::DescriptorSetLayout CreateDescriptorLayout(const vk::Device& device, const CreateDescriptorLayoutParams& params)
	{
		vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo = { static_cast<uint32_t>(params.bindingFlags.size()), params.bindingFlags.data() };

		const vk::DescriptorSetLayoutCreateInfo layoutInfo = { params.flags, static_cast<uint32_t>(params.bindings.size()), params.bindings.data(), &bindingFlagsCreateInfo };

		vk::DescriptorSetLayout layout;
		VK_CHECK(device.createDescriptorSetLayout(&layoutInfo, nullptr, &layout));

		return layout;
	}
}
