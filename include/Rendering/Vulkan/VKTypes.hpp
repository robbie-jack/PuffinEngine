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

	struct SwapchainData
	{
		void operator=(const SwapchainData& other)
		{
			swapchain = other.swapchain;

			imageFormat = other.imageFormat;
			images = other.images;
			imageViews = other.imageViews;

			framebuffers = other.framebuffers;

			depthImage = other.depthImage;

			needsCleaned = other.needsCleaned;
		}

		vk::SwapchainKHR swapchain;

		vk::Format imageFormat;
		std::vector<vk::Image> images;
		std::vector<vk::ImageView> imageViews;
		std::vector<vk::Framebuffer> framebuffers;

		AllocatedImage depthImage;

		bool needsCleaned = false;
	};

	struct MeshData
	{
		UUID assetID;

		AllocatedBuffer vertexBuffer;
		AllocatedBuffer indexBuffer;

		uint32_t numVertices;
		uint32_t numIndices;
	};

	struct TextureData
	{
		UUID assetID;
		int idx = 0;

		vk::Sampler sampler;
		Texture texture;
	};

	// GPU Data Structs

	struct GPUCameraData
	{
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
		alignas(16) glm::mat4 viewProj;
	};

	struct GPUObjectData
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 invModel;
		alignas(4) int texIndex;
	};

	struct GPULightData
	{
		alignas(4) glm::vec3 position;
		alignas(4) int pad1;
		alignas(4) glm::vec3 direction;
		alignas(4) int pad2;
		alignas(4) glm::vec3 color;
		alignas(4) int pad3;
		alignas(4) glm::vec3 ambientSpecular;
		alignas(4) int pad4;
		alignas(4) glm::vec3 attenuation;
		alignas(4) int pad5;
		alignas(4) glm::vec3 cutoffAngle;
		alignas(4) int type; // Stores type of light
	};

	struct GPULightStaticData
	{
		alignas(4) glm::vec3 viewPos;
		alignas(4) int numLights;
	};

}
