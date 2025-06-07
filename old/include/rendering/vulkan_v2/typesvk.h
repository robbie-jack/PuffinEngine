#pragma once

#include "vulkan/vulkan.hpp"

#include "vk_mem_alloc.hpp"

#include "glm/glm.hpp"
#include "imgui.h"
#include "puffin/rendering/materialglobals.h"
#include "puffin/types/uuid.h"

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
		UUID assetId;

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
		UUID id = gInvalidID;

		vk::Sampler sampler;
	};

	struct MaterialDataVK : AssetDataVK
	{
		UUID baseMaterialID = gInvalidID;

		int idx = 0;

		std::array<UUID, gNumTexturesPerMat> texIDs = { 0, 0, 0, 0, 0, 0, 0, 0 };
	};

	struct MaterialVK
	{
		UUID matID = gInvalidID;

		vk::UniquePipeline pipeline = {};
		vk::UniquePipelineLayout pipelineLayout = {};
	};

	struct MeshRenderable
	{
		MeshRenderable(const UUID entityID, const UUID meshID, const UUID matID, const uint8_t subMeshIdx = 0) :
			entityID(entityID), meshID(meshID), matID(matID), subMeshIdx(subMeshIdx) {}

		UUID entityID;
		UUID meshID;
		UUID matID;
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
		explicit MeshDrawBatch(const UUID matID = gInvalidID, const uint32_t cmdCount = 0, const uint32_t meshIndex = 0) :
			matID(matID), cmdCount(cmdCount), cmdIndex(meshIndex) {}

		UUID matID; // Id of this material
		uint32_t cmdCount = 0; // Number of commands to be drawn with this batch
		uint32_t cmdIndex = 0; // Starting index in indirect commands buffer for this batch

		bool operator<(const MeshDrawBatch& other) const
		{
			return matID < other.matID;
		}
	};

	// GPU Data Structs

	struct GPUShadowPushConstant
	{
		alignas(16) vk::DeviceAddress vertexBufferAddress;
		alignas(16) glm::mat4 lightSpaceView;
	};

	struct GPUVertexShaderPushConstant
	{
		alignas(16) vk::DeviceAddress vertexBufferAddress;
		alignas(16) glm::mat4 camViewProj;
	};

	struct GPUFragShaderPushConstant
	{
		alignas(16) glm::vec4 viewPos;
		alignas(16) glm::vec4 lightCount;
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
		alignas(4) glm::vec4 positionShadowIndex;
		alignas(4) glm::vec4 color;
		alignas(4) glm::vec4 ambientSpecularExponent;
	};

	struct GPUPointLightData : GPULightData
	{
		alignas(4) glm::vec4 attenuation;
	};

	struct GPUSpotLightData : GPULightData
	{
		alignas(4) glm::vec4 directionInnerCutoffAngle;
		alignas(4) glm::vec4 attenuationOuterCutoffAngle;
	};

	struct GPUDirLightData : GPULightData
	{
		alignas(4) glm::vec4 direction;
	};

	struct GPUShadowData
	{
		alignas(4) glm::vec4 shadowBiasCascadeIndexAndCount;
	};

    struct GPUShadowCascadeData
    {
        alignas(16) glm::mat4 lightSpaceView;
        alignas(4) float cascadePlaneDistance = 0.0f;
    };

	struct GPUMaterialInstanceData
	{
		int texIndices[gNumTexturesPerMat];
		float data[gNumFloatsPerMat];
	};

}
