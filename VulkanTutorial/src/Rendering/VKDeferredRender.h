#pragma once

#ifndef VK_DEFERRED_RENDER_H
#define VK_DEFERRED_RENDER_H

// Vulkan Helper Classes
#include <Rendering/VKTypes.h>
#include <Rendering/VKDescriptors.h>
#include <Rendering/VKPipeline.h>

// STL
#include <vector>
#include <fstream>

#include "vk_mem_alloc.h"  // for VmaAllocator
#include "vulkan/vulkan_core.h"      // for VkExtent2D, VkExtent3D, VkFormat, VkFramebuffer, VkImageUsageFlags

namespace Puffin
{
	namespace Rendering
	{
		struct DeferredFrameData
		{
			// G-Buffer
			VkFramebuffer gFramebuffer;
			AllocatedImage gPosition, gNormal, gAlbedoSpec;
			AllocatedImage gDepth;

			VkCommandBuffer gCommandBuffer, sCommandBuffer;

			// Synchronization
			VkSemaphore geometrySemaphore, shadingSemaphore;
		};

		class VKDeferredRender
		{
		public:

			/*
			* Setup Deferred Render
			* 
			* frameOverlap - number of frames to overlap when rendering
			*/
			void Setup(VkPhysicalDevice inPhysicalDevice, 
				VkDevice inDevice, 
				VmaAllocator inAllocator,
				VKUtil::DescriptorAllocator* inDescriptorAllocator,
				VKUtil::DescriptorLayoutCache* inDescriptorLayoutCache,
				VkDescriptorSetLayout inGeometrySetLayout,
				std::vector<VkCommandPool>& commandPools,
				int inFrameOverlap, VkExtent2D inExtent);

			// Pass Data Needed for Geometry Rendering
			inline void SetGeometryDescriptorSet(VkDescriptorSet* inGeometrySet)
			{
				geometrySet = inGeometrySet;
			}

			inline void SetDrawIndirectCommandsBuffer(IndirectDrawBatch* inIndirectDrawBatch)
			{
				indirectDrawBatch = inIndirectDrawBatch;
			}

			/*
			* Render Scene with deferred shading
			*/
			void DrawScene(int frameIndex, SceneData* sceneData, VkQueue graphicsQueue);

			// Cleanup Functions
			void Cleanup();

		private:

			// Variables

			// Vulkan Engine Handles
			VmaAllocator allocator;
			VkPhysicalDevice physicalDevice;
			VkDevice device;
			VKUtil::DescriptorAllocator* descriptorAllocator;
			VKUtil::DescriptorLayoutCache* descriptorLayoutCache;

			// Deletion Queue to cleanup up all Vulkan Objects
			DeletionQueue deletionQueue;

			// G-Buffer
			int frameOverlap = 2;
			std::vector<DeferredFrameData> frameData;

			VkExtent3D gBufferExtent;
			VkRenderPass gRenderPass;
			VkSampler gColorSampler;

			VkDescriptorSetLayout geometrySetLayout;

			VkPipeline gPipeline;
			VkPipelineLayout gPipelineLayout;

			VkDescriptorSet* geometrySet;
			IndirectDrawBatch* indirectDrawBatch;

			// Functions

			// Setup Functions
			void SetupGBuffer(); // Setup Geometry Framebuffer Attachments
			void SetupGRenderPass(); // Setup Geometry Render Pass
			void SetupGFramebuffer(); // Setup Geometry Framebuffer
			void SetupSynchronization(); // Setup Synchronization Objects
			void SetupGColorSampler(); // Setup Geometry Color Sampler
			void SetupCommandBuffers(std::vector<VkCommandPool>& commandPools); // Setup Command Pools/Buffers
			//void SetupDescriptorSets(); // Setup Descriptor Sets for Geometry Pass
			void SetupPipelines(); // Setup Geometry/Shading Pipelines

			/*
			* Create Allocated Images for the GBuffer
			*/
			void CreateAllocatedImage(VkFormat format, VkImageUsageFlags usage, 
				AllocatedImage* allocatedImage, std::string debug_name = "");

			// Draw Functions
			VkCommandBuffer RecordGeometryCommandBuffer(int frameIndex, SceneData* sceneData);
			VkCommandBuffer RecordShadingCommandBuffer(int frameIndex, SceneData* sceneData);

			static inline std::vector<char> ReadFile(const std::string& filename)
			{
				std::ifstream file(filename, std::ios::ate | std::ios::binary);

				if (!file.is_open())
				{
					throw std::runtime_error("failed to open file!");
				}

				size_t fileSize = (rsize_t)file.tellg();
				std::vector<char> buffer(fileSize);

				file.seekg(0);
				file.read(buffer.data(), fileSize);

				file.close();

				//std::cout << "BufferSize: " << buffer.size() << std::endl;

				return buffer;
			}
		};
	}
}

#endif // VK_DEFERRED_RENDER_H