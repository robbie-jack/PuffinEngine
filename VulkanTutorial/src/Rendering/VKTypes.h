#pragma once

#ifndef VULKAN_TYPES_H
#define VULKAN_TYPES_H

#include <vulkan/vulkan.h>
#include <Rendering/vk_mem_alloc.h>

#include <deque>
#include <functional>

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

		// <Merged Vertex and Index buffers for all the objects in the scene
		struct SceneData
		{
			AllocatedBuffer mergedVertexBuffer, mergedIndexBuffer;
			bool bFlagSceneChanged = true;
		};

		// Draw Commands and buffer for Indirect rendering
		struct IndirectDrawBatch
		{
			std::vector<VkDrawIndexedIndirectCommand> drawIndirectCommands;
			AllocatedBuffer drawIndirectCommandsBuffer;
		};
	}
}

#endif // VULKAN_TYPES_H