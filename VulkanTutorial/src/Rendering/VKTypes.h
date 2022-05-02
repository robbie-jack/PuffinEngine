#pragma once

#ifndef VULKAN_TYPES_H
#define VULKAN_TYPES_H

#include <vulkan/vulkan.h>
#include <Rendering/vk_mem_alloc.h>
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

		struct DeletionQueue
		{
			std::deque<std::function<void()>> deletors;

			void push_function(std::function<void()>&& function)
			{
				deletors.push_back(function);
			}

			void flush()
			{
				// reverse iterate the deletion queue to execute all the functions
				for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
					(*it)(); //call functors
				}

				deletors.clear();
			}
		};

		// Data needed for indirect rendering of a single mesh in scene
		struct MeshRenderData
		{
			UUID meshAssetID; // UUID of Mesh Asset

			uint32_t vertexCount = 0; // Number of Vertices in Mesh
			uint32_t indexCount = 0; // Number of Indices in Mesh

			uint32_t vertexOffset = 0; // Offset into merged Vertex Buffer
			uint32_t indexOffset = 0; // Offset into merged index buffer

			std::set<ECS::Entity> entities; // Entities using this mesh
		};

		struct TextureRenderData
		{
			UUID textureAssetID;

			Texture texture;

			std::set<ECS::Entity> entities; // Entities using this texture
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

		struct ShadingUBO
		{
			alignas(16) glm::vec3 viewPos;
			alignas(4) int displayDebugTarget;
		};
	}
}

#endif // VULKAN_TYPES_H