#include "puffin/rendering/vulkan/helpers_vk.h"

#include <iostream>
#include <algorithm>

#include "puffin/rendering/vulkan/render_system_vk.h"

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
	void immediate_submit(const std::shared_ptr<RenderSystemVK>& render_system,
	                     std::function<void(VkCommandBuffer cmd)>&& function)
	{
		vk::CommandBuffer cmd = render_system->upload_context().commandBuffer;

		const vk::CommandBufferBeginInfo cmdBeginInfo = { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
		VK_CHECK(cmd.begin(&cmdBeginInfo));

		function(cmd);

		cmd.end();

		const vk::SubmitInfo submit = { {}, {}, {}, 1, &cmd };

		VK_CHECK(render_system->graphics_queue().submit(1, &submit, render_system->upload_context().uploadFence));

		VK_CHECK(render_system->device().waitForFences(1, &render_system->upload_context().uploadFence, true, 9999999999));
		VK_CHECK(render_system->device().resetFences(1, &render_system->upload_context().uploadFence));

		render_system->device().resetCommandPool(render_system->upload_context().commandPool);
	}

	void copy_data_between_buffers(const std::shared_ptr<RenderSystemVK>& render_system, const vk::Buffer src_buffer,
	                               const vk::Buffer dst_buffer,
	                               const uint32_t data_size, const uint32_t src_offset, const uint32_t dst_offset)
	{
		immediate_submit(render_system, [=](const vk::CommandBuffer cmd)
		{
			vk::BufferCopy copy;
			copy.dstOffset = dst_offset;
			copy.srcOffset = src_offset;
			copy.size = data_size;
			cmd.copyBuffer(src_buffer, dst_buffer, 1, &copy);
		});
	}

	AllocatedBuffer create_buffer(const vma::Allocator& allocator, const size_t alloc_size, const vk::BufferUsageFlags usage,
	                             const vma::MemoryUsage memory_usage, const vma::AllocationCreateFlags alloc_flags, const vk::MemoryPropertyFlags required_flags)
	{
		const vk::BufferCreateInfo bufferInfo = { {}, alloc_size, usage };

		const vma::AllocationCreateInfo allocInfo = { alloc_flags, memory_usage, required_flags };

		AllocatedBuffer buffer;

		VK_CHECK(allocator.createBuffer(&bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &buffer.allocInfo));

		return buffer;
	}

	void copy_cpu_data_into_gpu_buffer(const std::shared_ptr<RenderSystemVK>& render_system, const AllocatedBuffer& dst_buffer, const uint32_t data_size,
	                                   const void* data, const uint32_t src_offset, const uint32_t dst_offset)
	{
		// If rebar is enabled and buffer is in host visible memory, copy directly to buffer
		if (const vk::MemoryPropertyFlags memPropFlags = render_system->allocator().getAllocationMemoryProperties(dst_buffer.allocation); render_system->rebar_enabled() 
			&& (memPropFlags & vk::MemoryPropertyFlagBits::eHostVisible))
		{
			const auto* dataChar = static_cast<const char*>(data);

			std::copy_n(dataChar, data_size, static_cast<char*>(dst_buffer.allocInfo.pMappedData) + dst_offset);
		}
		// If rebar is not enabled or buffer is not in host visible, copy data via staging buffer
		else
		{
			// Allocate Staging Buffer - Map Vertices in CPU Memory
			const AllocatedBuffer stagingBuffer = create_buffer(render_system->allocator(), data_size,
			                                                   vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eAuto, 
			                                                   { vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped });

			const auto* dataChar = static_cast<const char*>(data);
			auto* mappedDataChar = static_cast<char*>(stagingBuffer.allocInfo.pMappedData);

			std::copy_n(dataChar, data_size, mappedDataChar);

			// Copy from CPU Memory to GPU Memory
			copy_data_between_buffers(render_system, stagingBuffer.buffer, dst_buffer.buffer, data_size, src_offset, dst_offset);

			// Cleanup Staging Buffer Immediately, It is no longer needed
			render_system->allocator().destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);
		}
	}

	AllocatedBuffer init_vertex_buffer(const std::shared_ptr<RenderSystemVK>& render_system, const void* vertex_data, const size_t num_vertices, const size_t vertex_size)
	{
		// Copy Loaded Mesh data into mesh vertex buffer
		const uint32_t vertexBufferSize = num_vertices * vertex_size;

		// Allocate Vertex Buffer - Transfer Vertices into GPU Memory
		AllocatedBuffer vertexBuffer = create_buffer(render_system->allocator(), vertexBufferSize,
            { vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc }, 
			vma::MemoryUsage::eAuto, { vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead });

		copy_cpu_data_into_gpu_buffer(render_system, vertexBuffer, vertexBufferSize, vertex_data);

		return vertexBuffer;
	}

	AllocatedBuffer init_index_buffer(const std::shared_ptr<RenderSystemVK>& render_system, const void* index_data,
	                                  const size_t num_indices, const size_t index_size)
	{
		const uint32_t indexBufferSize = num_indices * index_size;

		// Allocate Index Buffer - Transfer indices into GPU memory
		AllocatedBuffer indexBuffer = create_buffer(render_system->allocator(), indexBufferSize,
			{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAuto, { vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead });

		copy_cpu_data_into_gpu_buffer(render_system, indexBuffer, indexBufferSize, index_data);

		return indexBuffer;
	}

	AllocatedImage create_image(const std::shared_ptr<RenderSystemVK>& render_system, const vk::ImageCreateInfo& image_info, vk::ImageViewCreateInfo image_view_info)
	{
		AllocatedImage allocImage;
		allocImage.format = image_info.format;

		// Create Image
		constexpr vma::AllocationCreateInfo imageAllocInfo = { {}, vma::MemoryUsage::eAutoPreferDevice, vk::MemoryPropertyFlagBits::eDeviceLocal };

		VK_CHECK(render_system->allocator().createImage(&image_info, &imageAllocInfo, &allocImage.image, &allocImage.allocation, &allocImage.allocInfo));

		// Create Image View
		image_view_info.image = allocImage.image;

		VK_CHECK(render_system->device().createImageView(&image_view_info, nullptr, &allocImage.imageView));

		return allocImage;
	}

	AllocatedImage init_depth_image(const std::shared_ptr<RenderSystemVK>& render_system, const vk::Extent3D extent, const vk::Format format)
	{
		const vk::ImageCreateInfo imageInfo = { {}, vk::ImageType::e2D, format, extent, 1, 1,
			vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment };

		constexpr vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 };

		const vk::ImageViewCreateInfo imageViewInfo({}, {}, vk::ImageViewType::e2D, format, {}, subresourceRange);

		return create_image(render_system, imageInfo, imageViewInfo);
	}

	AllocatedImage init_texture(const std::shared_ptr<RenderSystemVK>& render_system, const void* pixel_data, const uint32_t width, const uint32_t height, vk
	                            ::DeviceSize size, const vk::Format format)
	{
		// Allocate staging buffer on CPU for holding texture data to upload
		const AllocatedBuffer stagingBuffer = create_buffer(render_system->allocator(), size,
			vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eAuto, 
			vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

		// Copy texture data to buffer
		void* data;
		VK_CHECK(render_system->allocator().mapMemory(stagingBuffer.allocation, &data));
		memcpy(data, pixel_data, size);
		render_system->allocator().unmapMemory(stagingBuffer.allocation);

		// Allocate and create texture in GPU memory
		const vk::Extent3D imageExtent = { width, height, 1 };

		const vk::ImageCreateInfo imageInfo = { {}, vk::ImageType::e2D, format, imageExtent,
			1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eLinear,
			{ vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst } };

		constexpr vk::ImageSubresourceRange subresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		const vk::ImageViewCreateInfo imageViewInfo = { {}, {}, vk::ImageViewType::e2D, format, {}, subresourceRange };

		const AllocatedImage texture = create_image(render_system, imageInfo, imageViewInfo);

		// Fill command for copying pixels to texture and transitioning image layout
		immediate_submit(render_system, [=](vk::CommandBuffer cmd)
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
		render_system->allocator().destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);

		return texture;
	}
}
