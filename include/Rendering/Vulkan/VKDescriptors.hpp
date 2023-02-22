#pragma once

#include "Rendering/Vulkan/VKTypes.hpp"

#include <unordered_map>

namespace Puffin::Rendering::VK::Util
{
	class DescriptorAllocator
	{
	public:

		struct PoolSizes
		{
			std::vector<std::pair<vk::DescriptorType, float>> sizes =
			{
				{ vk::DescriptorType::eSampler, 0.5f },
				{ vk::DescriptorType::eCombinedImageSampler, 4.f },
				{ vk::DescriptorType::eSampledImage, 4.f },
				{ vk::DescriptorType::eStorageImage, 1.f },
				{ vk::DescriptorType::eUniformTexelBuffer, 1.f },
				{ vk::DescriptorType::eStorageTexelBuffer, 1.f },
				{ vk::DescriptorType::eUniformBuffer, 2.f },
				{ vk::DescriptorType::eStorageBuffer, 2.f },
				{ vk::DescriptorType::eUniformBufferDynamic, 1.f },
				{ vk::DescriptorType::eStorageBufferDynamic, 1.f },
				{ vk::DescriptorType::eInputAttachment, 0.5f }
			};
		};

		void Init(vk::Device device);

		void ResetPools();
		bool Allocate(vk::DescriptorSet* set, vk::DescriptorSetLayout layout);

		void Cleanup();

	private:

		vk::Device m_device;

		vk::DescriptorPool m_currentPool;

		PoolSizes descriptorSizes;

		std::vector<vk::DescriptorPool> m_usedPools;
		std::vector<vk::DescriptorPool> m_freePools;

		vk::DescriptorPool GrabPool();

	};

	class DescriptorLayoutCache
	{
	public:

		void Init(vk::Device device);
		void Cleanup();

		vk::DescriptorSetLayout CreateDescriptorLayout(vk::DescriptorSetLayoutCreateInfo* info);

		struct DescriptorLayoutInfo
		{
			std::vector<vk::DescriptorSetLayoutBinding> bindings;

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

		std::unordered_map<DescriptorLayoutInfo, vk::DescriptorSetLayout, DescriptorLayoutHash> m_layoutCache;

		vk::Device m_device;

	};

	class DescriptorBuilder
	{
	public:

		static DescriptorBuilder Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator);

		DescriptorBuilder& BindBuffer(uint32_t binding, vk::DescriptorBufferInfo* bufferInfo, vk::DescriptorType type, vk::ShaderStageFlags stageFlags);

		DescriptorBuilder& BindImage(uint32_t binding, vk::DescriptorImageInfo* imageInfo, vk::DescriptorType type, vk::ShaderStageFlags stageFlags);

		DescriptorBuilder& BindImages(uint32_t binding, uint32_t imageCount, vk::DescriptorImageInfo* imageInfos, vk::DescriptorType type, vk::ShaderStageFlags stageFlags);

		bool Build(vk::DescriptorSet& set, vk::DescriptorSetLayout& layout);
		bool Build(vk::DescriptorSet& set);

		// Used for writing to descriptor set which is already built
		DescriptorBuilder& UpdateImage(uint32_t binding, vk::DescriptorImageInfo* imageInfo,
			vk::DescriptorType type);

		DescriptorBuilder& UpdateImages(uint32_t binding, uint32_t imageCount,
			const vk::DescriptorImageInfo* imageInfos, vk::DescriptorType type);

		bool Update(vk::DescriptorSet& set);

	private:

		std::vector<vk::WriteDescriptorSet> m_writes;
		std::vector<vk::DescriptorSetLayoutBinding> m_bindings;

		DescriptorLayoutCache* m_cache;
		DescriptorAllocator* m_alloc;

	};
}