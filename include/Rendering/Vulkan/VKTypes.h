#pragma once

#include "vulkan/vulkan.hpp"

#include "vk_mem_alloc.hpp"

#include "glm/glm.hpp"
#include "imgui.h"
#include "puffin/types/uuid.h"
#include "Rendering/MaterialGlobals.h"

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

			needsCleaned = other.needsCleaned;
			resized = other.resized;
		}

		vk::SwapchainKHR swapchain;

		vk::Extent2D extent;
		vk::Format imageFormat;

		std::vector<vk::Image> images;
		std::vector<vk::ImageView> imageViews;

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

		bool operator<(const AssetDataVK& other) const
		{
			return assetId < other.assetId;
		}
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

	struct MaterialDataVK : AssetDataVK
	{
		PuffinID baseMaterialID = gInvalidID;

		int idx = 0;

		std::array<PuffinID, gNumTexturesPerMat> texIDs = { 0, 0, 0, 0, 0, 0, 0, 0 };
	};

	struct MaterialVK
	{
		PuffinID matID = gInvalidID;

		vk::UniquePipeline pipeline = {};
		vk::UniquePipelineLayout pipelineLayout = {};
	};

	struct MeshRenderable
	{
		MeshRenderable(const PuffinID entityID_, const PuffinID meshID_, const PuffinID matID_, const uint8_t subMeshIdx_ = 0) :
			entityID(entityID_), meshID(meshID_), matID(matID_), subMeshIdx(subMeshIdx_) {}

		PuffinID entityID;
		PuffinID meshID;
		PuffinID matID;
		uint8_t subMeshIdx = 0;

		bool operator<(const MeshRenderable& other) const
		{
			return matID < other.matID
			&& meshID < other.meshID
			&& subMeshIdx < other.subMeshIdx
			&& entityID < other.entityID;
		}
	};

	// Struct for rendering a batch of meshes which share a material
	struct MeshDrawBatch
	{
		MeshDrawBatch(const PuffinID matID_ = gInvalidID, const uint32_t cmdCount_ = 0, const uint32_t meshIndex_ = 0) :
			matID(matID_), cmdCount(cmdCount_), cmdIndex(meshIndex_) {}

		PuffinID matID; // Id of this material
		uint32_t cmdCount = 0; // Number of commands to be drawn with this batch
		uint32_t cmdIndex = 0; // Starting index in indirect commands buffer for this batch

		bool operator<(const MeshDrawBatch& other) const
		{
			return matID < other.matID;
		}
	};

	// GPU Data Structs

	struct GPUDrawPushConstant
	{
		alignas(16) vk::DeviceAddress vertexBufferAddress;
	};

	struct GPUCameraData
	{
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
		alignas(16) glm::mat4 viewProj;
	};

	struct GPUObjectData
	{
		alignas(16) glm::mat4 model;
		alignas(4) int matIdx;
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

	struct GPUMaterialInstanceData
	{
		int texIndices[gNumTexturesPerMat];
		float data[gNumFloatsPerMat];
	};

}
