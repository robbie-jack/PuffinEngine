#pragma once

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"

#include "ECS/EntityID.h"

#include <set>

namespace Puffin::Rendering::VK
{
	struct UploadContext
	{
		vk::Fence uploadFence;
		vk::CommandPool commandPool;
		vk::CommandBuffer commandBuffer;
	};

	struct AllocatedBuffer
	{
		vk::Buffer buffer;
		vma::Allocation allocation;
	};

	struct AllocatedImage
	{
		vk::Image image;
		vk::ImageView imageView;
		vk::Format format;
		vma::Allocation allocation;
	};

	typedef AllocatedImage Texture;

	struct MeshData
	{
		UUID assetID;

		AllocatedBuffer vertexBuffer;
		AllocatedBuffer indexBuffer;

		std::set<ECS::EntityID> entities;
	};
}
