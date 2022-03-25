#pragma once

#include <Rendering/VKTypes.h>
#include <Rendering/VKInitializers.h>
#include <Rendering/VulkanEngine.h>

//#ifndef STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
//#endif
#include <stb-master\stb_image.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace Puffin::IO
{
	static bool LoadImageFromFile(Rendering::VulkanEngine& engine, fs::path texturePath, Rendering::AllocatedImage& outImage)
	{
		int texWidth, texHeight, texChannels;

		stbi_uc* pixels = stbi_load(texturePath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels) {
			std::cout << "Failed to load texture file " << texturePath.string() << std::endl;
			return false;
		}

		void* pixel_ptr = pixels;
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		// The Format R8G8B8A8 exactly matches the pixels loaded through stb
		VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

		// Allocate Staging buffer for holding texture data to upload
		Rendering::AllocatedBuffer stagingBuffer = engine.CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		// Copy Texture Data to Buffer
		void* data;
		vmaMapMemory(engine.allocator, stagingBuffer.allocation, &data);
		memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));
		vmaUnmapMemory(engine.allocator, stagingBuffer.allocation);

		// Free Loaded Data, as pixels are now in staging buffer
		stbi_image_free(pixels);

		// Allocated and Create Texture Image
		VkExtent3D imageExtent =
		{
			static_cast<uint32_t>(texWidth),
			static_cast<uint32_t>(texHeight),
			1
		};

		VkImageCreateInfo imageInfo = Rendering::VKInit::ImageCreateInfo(imageFormat,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			imageExtent);

		Rendering::AllocatedImage newImage;

		VmaAllocationCreateInfo imageAllocInfo = {};
		imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		vmaCreateImage(engine.allocator, &imageInfo, &imageAllocInfo, &newImage.image, &newImage.allocation, nullptr);

		// Fill Command for transitioning texture image layout
		engine.ImmediateSubmit([&](VkCommandBuffer cmd)
		{
			VkImageSubresourceRange range;
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = 1;
			range.baseArrayLayer = 0;
			range.layerCount = 1;

			VkImageMemoryBarrier imageBarrier_toTransfer = {};
			imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

			imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier_toTransfer.image = newImage.image;
			imageBarrier_toTransfer.subresourceRange = range;

			imageBarrier_toTransfer.srcAccessMask = 0;
			imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			//barrier the image into the transfer-receive layout
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;

			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = imageExtent;

			//copy the buffer into the image
			vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;

			imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			//barrier the image into the shader readable layout
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

		});

		vmaDestroyBuffer(engine.allocator, stagingBuffer.buffer, stagingBuffer.allocation);

		outImage = newImage;

		return true;
	}

	static bool InitTextureImage(Rendering::VulkanEngine& engine, const std::vector<char>& pixels, const uint32_t texWidth, const uint32_t texHeight, Rendering::AllocatedImage& outImage)
	{
		void* pixel_ptr = (char*)pixels.data();
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		// The Format R8G8B8A8 exactly matches the pixels loaded through stb
		VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

		// Allocate Staging buffer for holding texture data to upload
		Rendering::AllocatedBuffer stagingBuffer = engine.CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		// Copy Texture Data to Buffer
		void* data;
		vmaMapMemory(engine.allocator, stagingBuffer.allocation, &data);
		memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));
		vmaUnmapMemory(engine.allocator, stagingBuffer.allocation);
		
		// Allocated and Create Texture Image
		VkExtent3D imageExtent =
		{
			static_cast<uint32_t>(texWidth),
			static_cast<uint32_t>(texHeight),
			1
		};

		VkImageCreateInfo imageInfo = Rendering::VKInit::ImageCreateInfo(imageFormat,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			imageExtent);

		Rendering::AllocatedImage newImage;

		VmaAllocationCreateInfo imageAllocInfo = {};
		imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		vmaCreateImage(engine.allocator, &imageInfo, &imageAllocInfo, &newImage.image, &newImage.allocation, nullptr);

		// Fill Command for transitioning texture image layout
		engine.ImmediateSubmit([&](VkCommandBuffer cmd)
			{
				VkImageSubresourceRange range;
				range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				range.baseMipLevel = 0;
				range.levelCount = 1;
				range.baseArrayLayer = 0;
				range.layerCount = 1;

				VkImageMemoryBarrier imageBarrier_toTransfer = {};
				imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

				imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageBarrier_toTransfer.image = newImage.image;
				imageBarrier_toTransfer.subresourceRange = range;

				imageBarrier_toTransfer.srcAccessMask = 0;
				imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				//barrier the image into the transfer-receive layout
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

				VkBufferImageCopy copyRegion = {};
				copyRegion.bufferOffset = 0;
				copyRegion.bufferRowLength = 0;
				copyRegion.bufferImageHeight = 0;

				copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.imageSubresource.mipLevel = 0;
				copyRegion.imageSubresource.baseArrayLayer = 0;
				copyRegion.imageSubresource.layerCount = 1;
				copyRegion.imageExtent = imageExtent;

				//copy the buffer into the image
				vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

				VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;

				imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				//barrier the image into the shader readable layout
				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

			});

		vmaDestroyBuffer(engine.allocator, stagingBuffer.buffer, stagingBuffer.allocation);

		outImage = newImage;

		return true;
	}
}