#pragma once

#include <unordered_map>
#include <memory>

#include "puffin/rendering/vulkan/vk_types.h"

namespace puffin::rendering::util
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

		DescriptorAllocator(const vk::Device device) : mDevice(device) {}
		~DescriptorAllocator();

		void resetPools();
		bool allocate(vk::DescriptorSet* set, vk::DescriptorSetLayout layout, const vk::BaseOutStructure* pNext = nullptr );

		void cleanup() const;

		const vk::Device& device() const { return mDevice; }

	private:

		vk::Device mDevice;

		vk::DescriptorPool mCurrentPool;

		PoolSizes mDescriptorSizes;

		std::vector<vk::DescriptorPool> mUsedPools;
		std::vector<vk::DescriptorPool> mFreePools;

		vk::DescriptorPool grabPool();

	};

	class DescriptorLayoutCache
	{
	public:

		DescriptorLayoutCache(const vk::Device device) : mDevice(device) {}
		~DescriptorLayoutCache();

		void cleanup() const;

		vk::DescriptorSetLayout createDescriptorLayout(const vk::DescriptorSetLayoutCreateInfo* info);

		struct DescriptorLayoutInfo
		{
			std::vector<vk::DescriptorSetLayoutBinding> bindings;

			bool operator==(const DescriptorLayoutInfo& other) const;

			[[nodiscard]] size_t hash() const;
		};

	private:

		struct DescriptorLayoutHash
		{
			std::size_t operator()(const DescriptorLayoutInfo& k) const
			{
				return k.hash();
			}
		};

		std::unordered_map<DescriptorLayoutInfo, vk::DescriptorSetLayout, DescriptorLayoutHash> mLayoutCache;

		vk::Device mDevice;

	};

	class DescriptorBuilder
	{
	public:

		DescriptorBuilder(const std::shared_ptr<DescriptorLayoutCache>& layoutCache, const std::shared_ptr<DescriptorAllocator>& allocator) : mCache(layoutCache), mAlloc(allocator) {}

		static DescriptorBuilder begin(const std::shared_ptr<DescriptorLayoutCache>& layoutCache, const std::shared_ptr<DescriptorAllocator>& allocator);

		DescriptorBuilder& bindBuffer(uint32_t binding, const vk::DescriptorBufferInfo* bufferInfo, vk::DescriptorType type, vk::ShaderStageFlags stageFlags);

		DescriptorBuilder& bindImage(const uint32_t binding, const vk::DescriptorImageInfo* imageInfo, const vk::DescriptorType type, const vk::
		                             ShaderStageFlags stageFlags);

		DescriptorBuilder& bindImages(const uint32_t binding, const uint32_t imageCount, const vk::DescriptorImageInfo* imageInfos, const vk::DescriptorType
		                              type, const vk::ShaderStageFlags stageFlags);

		DescriptorBuilder& bindImagesWithoutWrite(const uint32_t binding, const uint32_t imageCount, const vk::DescriptorType
										type, const vk::ShaderStageFlags stageFlags, vk::DescriptorBindingFlags bindingFlags = {});

		bool build(vk::DescriptorSet& set, vk::DescriptorSetLayout& layout);

		// Used for writing to descriptor set which is already built
		DescriptorBuilder& updateImage(uint32_t binding, const vk::DescriptorImageInfo* imageInfo,
			vk::DescriptorType type);

		DescriptorBuilder& updateImages(uint32_t binding, uint32_t imageCount,
			const vk::DescriptorImageInfo* imageInfos, vk::DescriptorType type);

		bool update(const vk::DescriptorSet& set);

		template<typename T>
		DescriptorBuilder& addPNext(T* structure)
		{
			mPNextChain.push_back(reinterpret_cast<vk::BaseOutStructure*>(structure));
			return *this;
		}

	private:

		std::vector<vk::WriteDescriptorSet> mWrites;
		std::vector<vk::DescriptorSetLayoutBinding> mBindings;
		std::vector<vk::DescriptorBindingFlags> mBindingFlags;
		std::vector<vk::BaseOutStructure*> mPNextChain;

		std::shared_ptr<DescriptorLayoutCache> mCache;
		std::shared_ptr<DescriptorAllocator> mAlloc;

	};
}
