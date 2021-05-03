#pragma once

#ifndef VK_DESCRIPTORS_H
#define VK_DESCRIPTORS_H

#include <Rendering/VKTypes.h>

// STL
#include <vector>
#include <array>
#include <unordered_map>

namespace Puffin
{
	namespace Rendering
	{
		namespace VKUtil
		{
			//////////////////////////////////////////////////
			// Descriptor Allocator
			//////////////////////////////////////////////////

			class DescriptorAllocator
			{
			public:

				struct PoolSizes
				{
					std::vector<std::pair<VkDescriptorType, float>> sizes =
					{
						{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
						{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
						{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
						{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
						{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
						{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
						{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
						{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
						{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
						{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
						{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
					};
				};

				void Init(VkDevice newDevice);

				void ResetPools();

				bool Allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);

				void Cleanup();

				VkDevice device;

			private:

				VkDescriptorPool GrabPool();

				VkDescriptorPool currentPool{ VK_NULL_HANDLE };
				PoolSizes descriptorSizes;
				std::vector<VkDescriptorPool> usedPools;
				std::vector<VkDescriptorPool> freePools;

			};

			//////////////////////////////////////////////////
			// Descriptor Layout Cache
			//////////////////////////////////////////////////

			class DescriptorLayoutCache
			{
			public:

				void Init(VkDevice newDevice);
				void Cleanup();

				VkDescriptorSetLayout CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo* info);

				struct DescriptorLayoutInfo
				{
					// Good idea to turn this into an inlined array
					std::vector<VkDescriptorSetLayoutBinding> bindings;

					bool operator==(const DescriptorLayoutInfo& other) const;

					size_t hash() const;
				};

			private:

				struct DescriptorLayoutHash
				{
					std::size_t operator()(const DescriptorLayoutInfo& k) const
					{
						return k.hash();
					}
				};

				// Consider more effecient hashmap for larger projects
				std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> layoutCache;
				VkDevice device;
			};

			//////////////////////////////////////////////////
			// Descriptor builder
			//////////////////////////////////////////////////

			class DescriptorBuilder
			{
			public:

				static DescriptorBuilder Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator);

				DescriptorBuilder& BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo,
					VkDescriptorType type, VkShaderStageFlags stageFlags);

				DescriptorBuilder& BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo,
					VkDescriptorType type, VkShaderStageFlags stageFlags);

				DescriptorBuilder& BindImages(uint32_t binding, 
					uint32_t imageCount, const VkDescriptorImageInfo* imageInfos,
					VkDescriptorType type, VkShaderStageFlags shaderFlags);

				bool Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
				bool Build(VkDescriptorSet& set);

				// Used for writing to descriptor set which is alreayd built
				DescriptorBuilder& UpdateImages(uint32_t binding, uint32_t imageCount,
					const VkDescriptorImageInfo* imageInfos, VkDescriptorType type);

				bool Update(VkDescriptorSet& set);

			private:

				std::vector<VkWriteDescriptorSet> writes;
				std::vector<VkDescriptorSetLayoutBinding> bindings;

				DescriptorLayoutCache* cache;
				DescriptorAllocator* alloc;

			};
		}
	}
}

#endif // VK_DESCRIPTORS_H