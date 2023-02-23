#pragma once

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"

#include "glm/glm.hpp"
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
	};

	struct GPUCameraData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewProj;
	};

	struct GPUObjectData
	{
		glm::mat4 model;
	};
}
