#pragma once

#include "vulkan/vulkan.hpp"

#include "vk_mem_alloc.hpp"

#include "glm/glm.hpp"
#include "imgui.h"
#include "puffin/rendering/material_globals.h"
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
			alloc_info = other.alloc_info;
		}

		vk::Buffer buffer;
		vma::Allocation allocation;
		vma::AllocationInfo alloc_info;
	};

	struct AllocatedImage
	{
		vk::Image image;
		vk::ImageView image_view;
		vk::Format format;
		vma::Allocation allocation;
		vma::AllocationInfo alloc_info;
	};

	using Texture = AllocatedImage;

	enum class ImageType
	{
		Color,
		Depth
	};

	struct ImageDesc
	{
		ImageType image_type = ImageType::Color;
		vk::Format format = vk::Format::eUndefined;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
	};

	struct SwapchainData
	{
		void operator=(const SwapchainData& other)
		{
			swapchain = other.swapchain;

			image_format = other.image_format;
			images = other.images;
			image_views = other.image_views;

			needs_cleaned = other.needs_cleaned;
			resized = other.resized;
		}

		vk::SwapchainKHR swapchain;

		vk::Extent2D extent;
		vk::Format image_format;

		std::vector<vk::Image> images;
		std::vector<vk::ImageView> image_views;

		bool needs_cleaned = false;
		bool resized = false;
	};

	struct OffscreenData
	{
		void operator=(const OffscreenData& other)
		{
			extent = other.extent;
			image_format = other.image_format;

			alloc_images = other.alloc_images;
			viewport_textures = other.viewport_textures;

			alloc_depth_image = other.alloc_depth_image;

			needs_cleaned = other.needs_cleaned;
			resized = other.resized;
		}

		vk::Extent2D extent;
		vk::Format image_format;

		std::vector<AllocatedImage> alloc_images;

		std::vector<ImTextureID> viewport_textures;

		AllocatedImage alloc_depth_image;

		bool needs_cleaned = false;
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

	struct GPUShadowPushConstant
	{
		alignas(16) vk::DeviceAddress vertex_buffer_address;
		alignas(16) glm::mat4 light_space_view;
	};

	struct GPUVertexShaderPushConstant
	{
		alignas(16) vk::DeviceAddress vertex_buffer_address;
	};

	struct GPUCameraData
	{
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
		alignas(16) glm::mat4 view_proj;
	};

	struct GPUObjectData
	{
		alignas(16) glm::mat4 model;
		alignas(4) int mat_idx;
	};

	struct GPULightData
	{
		// Light Data
		alignas(4) glm::vec4 position_and_type;
		alignas(4) glm::vec4 direction;
		alignas(4) glm::vec4 color;
		alignas(4) glm::vec4 ambient_specular;
		alignas(4) glm::vec4 attenuation;
		alignas(4) glm::vec4 cutoff_angle_and_shadow_index;

		// Shadow Data
		alignas(4) glm::vec4 shadow_bias;
		alignas(16) glm::mat4 light_space_view;
	};

	struct GPULightStaticData
	{
		alignas(16) glm::vec4 view_pos_and_light_count;
	};

	struct GPUMaterialInstanceData
	{
		int tex_indices[gNumTexturesPerMat];
		float data[gNumFloatsPerMat];
	};

}
