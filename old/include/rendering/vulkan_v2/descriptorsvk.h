#pragma once

#include <unordered_map>
#include <memory>

#include "puffin/rendering/vulkan/typesvk.h"

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

		explicit DescriptorAllocator(const vk::Device device) : mDevice(device) {}
		~DescriptorAllocator();

		void ResetPools();
		bool Allocate(vk::DescriptorSet* set, vk::DescriptorSetLayout layout, const vk::BaseOutStructure* pNext = nullptr );

		void Cleanup() const;

		const vk::Device& GetDevice() const;

	private:

		vk::DescriptorPool GrabPool();

		vk::Device mDevice;

		vk::DescriptorPool mCurrentPool;

		PoolSizes mDescriptorSizes;

		std::vector<vk::DescriptorPool> mUsedPools;
		std::vector<vk::DescriptorPool> mFreePools;

	};

	class DescriptorLayoutCache
	{
	public:

		explicit DescriptorLayoutCache(const vk::Device device) : mDevice(device) {}
		~DescriptorLayoutCache();

		void Cleanup() const;

		vk::DescriptorSetLayout CreateDescriptorLayout(const vk::DescriptorSetLayoutCreateInfo* info);

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

		DescriptorBuilder(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator);

		static DescriptorBuilder Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator);

		DescriptorBuilder& BindBuffer(uint32_t binding, const vk::DescriptorBufferInfo* bufferInfo, vk::DescriptorType type, vk::ShaderStageFlags stageFlags);

		DescriptorBuilder& BindImage(uint32_t binding, const vk::DescriptorImageInfo* imageInfo, vk::DescriptorType type, vk::ShaderStageFlags stageFlags);

		DescriptorBuilder& BindImages(uint32_t binding, uint32_t imageCount, const vk::DescriptorImageInfo* imageInfos, vk::DescriptorType type, vk::ShaderStageFlags stageFlags);

		DescriptorBuilder& BindImagesWithoutWrite(uint32_t binding, uint32_t imageCount, vk::DescriptorType type, vk::ShaderStageFlags stageFlags, vk::DescriptorBindingFlags bindingFlags = {});

		bool Build(vk::DescriptorSet& set, vk::DescriptorSetLayout& layout);

		// Used for writing to descriptor set which is already built
		DescriptorBuilder& UpdateImage(uint32_t binding, const vk::DescriptorImageInfo* imageInfo,
			vk::DescriptorType type);

		DescriptorBuilder& UpdateImages(uint32_t binding, uint32_t imageCount,
			const vk::DescriptorImageInfo* imageInfos, vk::DescriptorType type);

		bool Update(const vk::DescriptorSet& set);

		template<typename T>
		DescriptorBuilder& AddPNext(T* structure)
		{
			mPNextChain.push_back(reinterpret_cast<vk::BaseOutStructure*>(structure));
			return *this;
		}

	private:

		std::vector<vk::WriteDescriptorSet> mWrites;
		std::vector<vk::DescriptorSetLayoutBinding> mBindings;
		std::vector<vk::DescriptorBindingFlags> mBindingFlags;
		std::vector<vk::BaseOutStructure*> mPNextChain;

		DescriptorLayoutCache* mCache;
		DescriptorAllocator* mAlloc;

	};
}
