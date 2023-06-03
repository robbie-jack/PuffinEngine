#pragma once

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.hpp"

#include "glm/glm.hpp"
#include "imgui.h"
#include "Types/UUID.h"

namespace puffin::rendering
{
	struct UploadContext
	{
		vk::Fence uploadFence;
		vk::CommandPool commandPool;
		vk::CommandBuffer commandBuffer;
	};

	struct AllocatedBuffer
	{
		void operator=(const AllocatedBuffer& other)
		{
			buffer = other.buffer;
			allocation = other.allocation;
			allocInfo = other.allocInfo;
		}

		vk::Buffer buffer;
		vma::Allocation allocation;
		vma::AllocationInfo allocInfo;
	};

	struct AllocatedImage
	{
		vk::Image image;
		vk::ImageView imageView;
		vk::Format format;
		vma::Allocation allocation;
		vma::AllocationInfo allocInfo;
	};

	using Texture = AllocatedImage;

	struct SwapchainData
	{
		void operator=(const SwapchainData& other)
		{
			swapchain = other.swapchain;

			imageFormat = other.imageFormat;
			images = other.images;
			imageViews = other.imageViews;
			framebuffers = other.framebuffers;

			needsCleaned = other.needsCleaned;
			resized = other.resized;
		}

		vk::SwapchainKHR swapchain;

		vk::Extent2D extent;
		vk::Format imageFormat;

		std::vector<vk::Image> images;
		std::vector<vk::ImageView> imageViews;
		std::vector<vk::Framebuffer> framebuffers;

		bool needsCleaned = false;
		bool resized = false;
	};

	struct OffscreenData
	{
		void operator=(const OffscreenData& other)
		{
			extent = other.extent;
			imageFormat = other.imageFormat;

			allocImages = other.allocImages;
			viewportTextures = other.viewportTextures;

			allocDepthImage = other.allocDepthImage;

			needsCleaned = other.needsCleaned;
			resized = other.resized;
		}

		vk::Extent2D extent;
		vk::Format imageFormat;

		std::vector<AllocatedImage> allocImages;

		std::vector<ImTextureID> viewportTextures;

		AllocatedImage allocDepthImage;

		bool needsCleaned = false;
		bool resized = false;
	};

	struct AssetDataVK
	{
		PuffinID assetId;
	};

	struct MeshDataVK : AssetDataVK
	{
		AllocatedBuffer vertexBuffer;
		AllocatedBuffer indexBuffer;

		uint32_t numVertices;
		uint32_t numIndices;
	};

	struct TextureDataVK : AssetDataVK
	{
		int idx = 0;

		vk::Sampler sampler;
		Texture texture;
	};

	struct MeshRenderable
	{
		MeshRenderable(const PuffinID entityID_, const PuffinID meshID_) : entityID(entityID_), meshID(meshID_) {}

		PuffinID entityID;
		PuffinID meshID;

		bool operator<(const MeshRenderable& other) const
		{
			return meshID < other.meshID && entityID < other.entityID;
		}
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
		alignas(4) int texIndex;
	};

	struct GPULightData
	{
		alignas(16) glm::vec4 positionAndType;
		alignas(16) glm::vec4 direction;
		alignas(16) glm::vec4 color;
		alignas(16) glm::vec4 ambientSpecular;
		alignas(16) glm::vec4 attenuation;
		alignas(16) glm::vec4 cutoffAngle;
	};

	struct GPULightStaticData
	{
		alignas(16) glm::vec4 viewPosAndNumLights;
	};

	constexpr uint16_t gNumTexturesPerMat = 8;
	constexpr uint16_t gNumFloatsPerMat = 8;

	struct GPUMaterialInstanceData
	{
		int texIndices[gNumTexturesPerMat];
		float data[gNumFloatsPerMat];
	};

}
