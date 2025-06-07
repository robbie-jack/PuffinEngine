#pragma once

#ifndef VULKAN_TYPES_H
#define VULKAN_TYPES_H

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <glm/glm.hpp>

#include <deque>
#include <functional>
#include <map>
#include <set>

#include "ECS/Entity.h"
#include "Types/UUID.h"

namespace Puffin
{
	namespace Rendering
	{
		// Allocated Image/View
		struct AllocatedImage
		{
			VkImage image;
			VmaAllocation allocation;
			VkImageView imageView;
			VkFormat format;
		};

		typedef AllocatedImage Texture;

		struct AllocatedBuffer
		{
			VkBuffer buffer;
			VmaAllocation allocation;
		};

		struct Material
		{
			VkDescriptorSet textureSet;
			VkPipeline pipeline;
			VkPipelineLayout pipelineLayout;
		};

		// Data needed for indirect rendering of a single mesh in scene
		struct MeshRenderData
		{
			UUID meshAssetID; // UUID of Mesh Asset

			uint32_t vertexCount = 0; // Number of Vertices in Mesh
			uint32_t indexCount = 0; // Number of Indices in Mesh

			uint32_t vertexOffset = 0; // Offset into merged Vertex Buffer
			uint32_t indexOffset = 0; // Offset into merged index buffer

			std::set<ECS::EntityID> entities; // Entities using this mesh
		};

		// Data needed to copy vertex/index data to buffer
		struct MeshBufferData
		{
			void* vertexData = nullptr; // Pointer to Vertex Data
			void* indexData = nullptr; // Pointer to Index Data

			uint32_t vertexSize = 0; // Size of individual vertex
			uint32_t indexSize = 0; // Size of individual index
		};

		struct TextureRenderData
		{
			UUID textureAssetID;

			Texture texture;

			std::set<ECS::EntityID> entities; // Entities using this texture
		};

		// Merged Vertex and Index buffers for all the objects in the scene
		struct SceneRenderData
		{
			AllocatedBuffer mergedVertexBuffer, mergedIndexBuffer; // Merged Vertex/Index Buffers

			uint32_t vertexBufferSize = 750000; // Size of current Merged Vertex Buffers
			uint32_t indexBufferSize = 300000; // Size of current Merged Index Buffers

			uint32_t vertexOffset = 0; // Offset into merged Vertex Buffer
			uint32_t indexOffset = 0; // Offset into merged index buffer

			std::unordered_map<UUID, MeshRenderData> meshRenderDataMap; // Map of Mesh Render Data Structs
			std::unordered_map<UUID, TextureRenderData> albedoTextureData; // Map of Textures
		};

		// Draw Commands and buffer for Indirect rendering
		struct IndirectDrawBatch
		{
			AllocatedBuffer drawIndirectCommandsBuffer; // Buffer containing draw commands
			uint32_t count; // Number of commands mapped to buffer
		};

		struct GPUObjectData
		{
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 inv_model;
		};

		struct GPUInstanceData
		{
			alignas(4) int objectOffset; // Object Buffer Offset
		};

		struct GPUCameraData
		{
			alignas(16) glm::mat4 viewProj;
		};

		struct GPULightSpaceData
		{
			alignas(16) glm::mat4 lightSpaceMatrix;
		};

		struct GPULightIndexData
		{
			alignas(4) int lightSpaceIndex;
		};

		struct GPUShadingData
		{
			alignas(16) glm::vec3 viewPos;
			alignas(4) int displayDebugTarget;
		};

		struct GPULightData
		{
			alignas(16) glm::vec3 ambientColor;
			alignas(16) glm::vec3 diffuseColor;

			alignas(4) float specularStrength;
			alignas(4) int shininess;

			alignas(16) glm::mat4 lightSpaceMatrix;
			alignas(4) int shadowmapIndex;
		};

		struct GPUPointLightData
		{
			alignas(16) glm::vec3 position;

			alignas(4) float constant;
			alignas(4) float linear;
			alignas(4) float quadratic;

			alignas(4) int dataIndex;
		};

		struct GPUDirLightData
		{
			alignas(16) glm::vec3 direction;

			alignas(4) int dataIndex;
		};

		struct GPUSpotLightData
		{
			alignas(16) glm::vec3 position;
			alignas(16) glm::vec3 direction;

			alignas(4) float innerCutoff;
			alignas(4) float outerCutoff;

			alignas(4) float constant;
			alignas(4) float linear;
			alignas(4) float quadratic;

			alignas(4) int dataIndex;
		};

		struct GPULightStatsData
		{
			alignas(4) int numPLights;
			alignas(4) int numDLights;
			alignas(4) int numSLights;
		};
	}
}

#endif // VULKAN_TYPES_H