#pragma once

#ifndef VK_DEFERRED_RENDER_H
#define VK_DEFERRED_RENDER_H

// Vulkan Helper Classes
#include <Rendering/Vulkan/VKTypes.h>
#include <Rendering/Vulkan/Helpers/VKDescriptors.h>

#include "vma/vk_mem_alloc.h"  // for VmaAllocator
#include "vulkan/vulkan_core.h"      // for VkExtent2D, VkExtent3D, VkFormat, VkFramebuffer, VkImageUsageFlags

#include "Types/DeletionQueue.hpp"

// STL
#include <vector>
#include <fstream>

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

			// Shading
			VkDescriptorSet gBufferDescriptor;

			// Command Buffers
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
			           std::vector<VkCommandPool>& commandPools,
			           int inFrameOverlap,
			           VkExtent2D inExtent);

			/* 
			* Setup Deferred Geometry Pass
			*/
			void SetupGeometry(VkDescriptorSetLayout& inCameraVPSetLayout, VkDescriptorSetLayout& inObjectInstanceSetLayout, VkDescriptorSetLayout& inMatTextureSetLayout);

			/*
			* Setup Deferred Shading Pass
			*/
			void SetupShading(VkRenderPass inRenderPass, VkDescriptorSetLayout& inCameraShadingSetLayout, VkDescriptorSetLayout& inLightSetLayout, VkDescriptorSetLayout& inShadowmapSetLayout);

			void RecreateFramebuffer(VkExtent2D inExtent);

			// Pass Data Needed for Geometry Rendering
			// Pass Data Needed for Geometry Rendering
			inline void SetGeometryDescriptorSets(VkDescriptorSet* inCameraVPSet, VkDescriptorSet* inObjectInstanceSet, VkDescriptorSet* inMatTextureSet)
			{
				m_cameraVPSet = inCameraVPSet;
				m_modelInstanceSet = inObjectInstanceSet;
				m_matTextureSet = inMatTextureSet;
			}

			inline void SetShadingDescriptorSets(VkDescriptorSet* inCameraShadingSet, VkDescriptorSet* inLightDataSet, VkDescriptorSet* inShadowmapSet)
			{
				m_cameraShadingSet = inCameraShadingSet;
				m_lightDataSet = inLightDataSet;
				m_shadowmapSet = inShadowmapSet;
			}

			inline void SetDrawIndirectCommandsBuffer(IndirectDrawBatch* inIndirectDrawBatch)
			{
				indirectDrawBatch = inIndirectDrawBatch;
			}

			/*
			* Render Scene with deferred shading
			*/
			VkSemaphore& DrawScene(int frameIndex, SceneRenderData* sceneData, VkQueue graphicsQueue, VkFramebuffer sFramebuffer, VkSemaphore& shadowmapWaitSemaphore);

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
			DeletionQueue m_deletionQueue;
			DeletionQueue m_framebufferDeletionQueue;

			// G-Buffer
			int frameOverlap = 2;
			std::vector<DeferredFrameData> frameData;

			VkExtent3D gBufferExtent;
			VkRenderPass gRenderPass;
			VkSampler gColorSampler;

			// Geometry
			VkDescriptorSetLayout m_cameraVPLayout;
			VkDescriptorSetLayout m_objectInstanceLayout;
			VkDescriptorSetLayout m_matTextureLayout;

			VkPipeline gPipeline;
			VkPipelineLayout gPipelineLayout;

			VkDescriptorSet* m_cameraVPSet = nullptr;
			VkDescriptorSet* m_modelInstanceSet = nullptr;
			VkDescriptorSet* m_matTextureSet = nullptr;

			IndirectDrawBatch* indirectDrawBatch;

			// Shading
			VkRenderPass sRenderPass;

			VkDescriptorSetLayout m_gBufferLayout;
			VkDescriptorSetLayout m_cameraShadingLayout;
			VkDescriptorSetLayout m_lightLayout;
			VkDescriptorSetLayout m_shadowmapLayout;

			VkDescriptorSet* m_cameraShadingSet = nullptr;
			VkDescriptorSet* m_lightDataSet = nullptr;
			VkDescriptorSet* m_shadowmapSet = nullptr;

			VkPipeline sPipeline;
			VkPipelineLayout sPipelineLayout;

			// Functions

			// Setup Functions

			// Geometry Stage
			void SetupGBuffer(); // Setup Geometry Framebuffer Attachments
			void SetupGRenderPass(); // Setup Geometry Render Pass
			void SetupGFramebuffer(); // Setup Geometry Framebuffer
			void SetupSynchronization(); // Setup Synchronization Objects
			void SetupGColorSampler(); // Setup Geometry Color Sampler
			void SetupCommandBuffers(std::vector<VkCommandPool>& commandPools); // Setup Command Pools/Buffers
			void SetupGPipeline(); // Setup Geometry/Shading Pipelines

			// Shading Stage
			void SetupGBufferDescriptorSets(); // Setup Descriptor Sets for Shading Pass
			void SetupSPipeline();

			void UpdateGBufferDescriptorSets(); // Update GBuffer Image data in Shading Descriptor Set

			/*
			* Create Allocated Images for the GBuffer
			*/
			void CreateAllocatedImage(VkFormat format, VkImageUsageFlags usage, 
				AllocatedImage* allocatedImage, std::string debug_name = "");

			// Draw Functions
			VkCommandBuffer RecordGeometryCommandBuffer(int frameIndex, SceneRenderData* sceneData);
			VkCommandBuffer RecordShadingCommandBuffer(int frameIndex, SceneRenderData* sceneData, VkFramebuffer sFramebuffer);

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