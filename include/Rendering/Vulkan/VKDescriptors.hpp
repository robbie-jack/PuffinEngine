#pragma once

#include "Rendering/Vulkan/VKTypes.hpp"

#include <unordered_map>
#include <memory>

namespace puffin::Rendering::VK::Util
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

		DescriptorAllocator(vk::Device device) : m_device(device) {}
		~DescriptorAllocator();

		void ResetPools();
		bool Allocate(vk::DescriptorSet* set, vk::DescriptorSetLayout layout);

		void Cleanup();

		const vk::Device& Device() { return m_device; }

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

		DescriptorLayoutCache(vk::Device device) : m_device(device) {}
		~DescriptorLayoutCache();

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

		DescriptorBuilder(std::shared_ptr<DescriptorLayoutCache> layoutCache, std::shared_ptr<DescriptorAllocator> allocator) : m_cache(layoutCache), m_alloc(allocator) {}

		static DescriptorBuilder Begin(std::shared_ptr<DescriptorLayoutCache> layoutCache, std::shared_ptr<DescriptorAllocator> allocator);

		DescriptorBuilder& BindBuffer(uint32_t binding, vk::DescriptorBufferInfo* bufferInfo, vk::DescriptorType type, vk::ShaderStageFlags stageFlags);

		DescriptorBuilder& BindImage(uint32_t binding, vk::DescriptorImageInfo* imageInfo, vk::DescriptorType type, vk::ShaderStageFlags stageFlags);

		DescriptorBuilder& BindImages(uint32_t binding, uint32_t imageCount, vk::DescriptorImageInfo* imageInfos, vk::DescriptorType type, vk::ShaderStageFlags stageFlags);

		bool Build(vk::DescriptorSet& set, vk::DescriptorSetLayout& layout);

		// Used for writing to descriptor set which is already built
		DescriptorBuilder& UpdateImage(uint32_t binding, vk::DescriptorImageInfo* imageInfo,
			vk::DescriptorType type);

		DescriptorBuilder& UpdateImages(uint32_t binding, uint32_t imageCount,
			const vk::DescriptorImageInfo* imageInfos, vk::DescriptorType type);

		bool Update(vk::DescriptorSet& set);

	private:

		std::vector<vk::WriteDescriptorSet> m_writes;
		std::vector<vk::DescriptorSetLayoutBinding> m_bindings;

		std::shared_ptr<DescriptorLayoutCache> m_cache;
		std::shared_ptr<DescriptorAllocator> m_alloc;

	};
}