#include "Rendering\Vulkan\VKDescriptors.h"

#include <iostream>

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		vk::Result err = x;                                         \
		if (err != vk::Result::eSuccess)                            \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

namespace puffin::rendering::util
{
	vk::DescriptorPool CreatePool(vk::Device device, const DescriptorAllocator::PoolSizes& poolSizes, 
		int count, vk::DescriptorPoolCreateFlags createFlags = {})
	{
		std::vector<vk::DescriptorPoolSize> sizes;
		sizes.reserve(poolSizes.sizes.size());

		for (auto sz : poolSizes.sizes)
		{
			sizes.push_back( { sz.first, static_cast<uint32_t>(sz.second * count) } );
		}

		const vk::DescriptorPoolCreateInfo poolInfo = { createFlags, static_cast<uint32_t>(count), static_cast<uint32_t>(sizes.size()), sizes.data() };

		vk::DescriptorPool pool;
		VK_CHECK(device.createDescriptorPool(&poolInfo, nullptr, &pool));

		return pool;
	}

	DescriptorAllocator::~DescriptorAllocator()
	{
		cleanup();
	}

	void DescriptorAllocator::resetPools()
	{
		for (auto pool : mUsedPools)
		{
			mDevice.resetDescriptorPool(pool, {});
			mFreePools.push_back(pool);
		}

		mUsedPools.clear();

		mCurrentPool = nullptr;
	}

	bool DescriptorAllocator::allocate(vk::DescriptorSet* set, vk::DescriptorSetLayout layout)
	{
		if (!mCurrentPool)
		{
			mCurrentPool = grabPool();
			mUsedPools.push_back(mCurrentPool);
		}

		const vk::DescriptorSetAllocateInfo allocInfo = { mCurrentPool, 1, &layout };

		vk::Result allocResult = mDevice.allocateDescriptorSets(&allocInfo, set);
		bool needReallocate = false;

		switch (allocResult)
		{
		case vk::Result::eSuccess:
			
			return true;

		case vk::Result::eErrorFragmentedPool:

			needReallocate = true;
			break;

		case vk::Result::eErrorOutOfPoolMemory:

			needReallocate = true;
			break;

		default:

			return false;
		}

		if (needReallocate)
		{
			mCurrentPool = grabPool();

			mUsedPools.push_back(mCurrentPool);

			allocResult = mDevice.allocateDescriptorSets(&allocInfo, set);

			if (allocResult == vk::Result::eSuccess)
			{
				return true;
			}
		}

		return false;
	}

	void DescriptorAllocator::cleanup() const
	{
		for (const auto pool : mFreePools)
		{
			mDevice.destroyDescriptorPool(pool, nullptr);
		}

		for (const auto pool : mUsedPools)
		{
			mDevice.destroyDescriptorPool(pool, nullptr);
		}
	}

	vk::DescriptorPool DescriptorAllocator::grabPool()
	{
		// Check if there are reusable pools available
		if (mFreePools.size() > 0)
		{
			// Grab pool from free pools
			const vk::DescriptorPool pool = mFreePools.back();
			mFreePools.pop_back();

			return pool;
		}
		else
		{
			// Create new pool
			return CreatePool(mDevice, mDescriptorSizes, 1000, {});
		}
	}

	DescriptorLayoutCache::~DescriptorLayoutCache()
	{
		cleanup();
	}

	void DescriptorLayoutCache::cleanup() const
	{
		for (const auto pair : mLayoutCache)
		{
			mDevice.destroyDescriptorSetLayout(pair.second, nullptr);
		}
	}

	vk::DescriptorSetLayout DescriptorLayoutCache::createDescriptorLayout(const vk::DescriptorSetLayoutCreateInfo* info)
	{
		DescriptorLayoutInfo layoutInfo;
		layoutInfo.bindings.reserve(info->bindingCount);

		bool isSorted = true;
		int lastBinding = -1;

		// Copy from direct info struct into our own
		for (int i = 0; i < info->bindingCount; i++)
		{
			layoutInfo.bindings.push_back(info->pBindings[i]);

			// Check that bindings are in strict increasing order
			if (info->pBindings[i].binding > lastBinding)
			{
				lastBinding = info->pBindings[i].binding;
			}
			else
			{
				isSorted = false;
			}
		}

		// Sort bindings if they are out of order
		if (!isSorted)
		{
			std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(),
				[](const vk::DescriptorSetLayoutBinding& a, const vk::DescriptorSetLayoutBinding& b)
				{
					return a.binding < b.binding;
				});
		}

		// Try to grab from cache
		const auto it = mLayoutCache.find(layoutInfo);
		if (it != mLayoutCache.end())
		{
			return (*it).second;
		}
		else
		{
			vk::DescriptorSetLayout layout;
			VK_CHECK(mDevice.createDescriptorSetLayout(info, nullptr, &layout));

			// Add to cache
			mLayoutCache[layoutInfo] = layout;
			return layout;
		}
	}

	bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
	{
		if (other.bindings.size() != bindings.size()) {
			return false;
		}
		else {
			//compare each of the bindings is the same. Bindings are sorted so they will match
			for (int i = 0; i < bindings.size(); i++) {
				if (other.bindings[i].binding != bindings[i].binding) {
					return false;
				}
				if (other.bindings[i].descriptorType != bindings[i].descriptorType) {
					return false;
				}
				if (other.bindings[i].descriptorCount != bindings[i].descriptorCount) {
					return false;
				}
				if (other.bindings[i].stageFlags != bindings[i].stageFlags) {
					return false;
				}
			}
			return true;
		}
	}

	size_t DescriptorLayoutCache::DescriptorLayoutInfo::hash() const
	{
		using std::size_t;
		using std::hash;

		size_t result = hash<size_t>()(bindings.size());

		for (const VkDescriptorSetLayoutBinding& b : bindings)
		{
			//pack the binding data into a single int64. Not fully correct but it's ok
			size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

			//shuffle the packed binding data and xor it with the main hash
			result ^= hash<size_t>()(binding_hash);
		}

		return result;
	}

	DescriptorBuilder DescriptorBuilder::begin(const std::shared_ptr<DescriptorLayoutCache>& layoutCache, const std::shared_ptr<DescriptorAllocator>& allocator)
	{
		return DescriptorBuilder{ layoutCache, allocator };
	}

	DescriptorBuilder& DescriptorBuilder::bindBuffer(const uint32_t binding, const vk::DescriptorBufferInfo* bufferInfo,
	                                                 const vk::DescriptorType type, const vk::ShaderStageFlags stageFlags)
	{
		mBindings.emplace_back(binding, type, 1, stageFlags);
		mWrites.emplace_back(vk::WriteDescriptorSet{ {}, binding, 0, 1, type, nullptr, bufferInfo });

		return *this;
	}

	DescriptorBuilder& DescriptorBuilder::bindImage(const uint32_t binding, const vk::DescriptorImageInfo* imageInfo,
	                                                const vk::DescriptorType type, const vk::ShaderStageFlags stageFlags)
	{
		mBindings.emplace_back(binding, type, 1, stageFlags );
		mWrites.emplace_back(vk::WriteDescriptorSet{ {}, binding, 0, 1, type, imageInfo, nullptr });

		return *this;
	}

	DescriptorBuilder& DescriptorBuilder::bindImages(const uint32_t binding, const uint32_t imageCount,
	                                                 const vk::DescriptorImageInfo* imageInfos, const vk::DescriptorType type, const vk::ShaderStageFlags stageFlags)
	{
		mBindings.emplace_back(binding, type, imageCount, stageFlags);
		mWrites.emplace_back(vk::WriteDescriptorSet{ {}, binding, 0, imageCount, type, imageInfos, nullptr });

		return *this;
	}

	bool DescriptorBuilder::build(vk::DescriptorSet& set, vk::DescriptorSetLayout& layout)
	{
		const vk::DescriptorSetLayoutCreateInfo layoutInfo = { {}, static_cast<uint32_t>(mBindings.size()), mBindings.data() };

		layout = mCache->createDescriptorLayout(&layoutInfo);

		// Allocate Descriptor
		if (const bool success = mAlloc->allocate(&set, layout); !success) { return false; }

		// Write Descriptor
		for (auto& w : mWrites)
		{
			w.dstSet = set;
		}

		mAlloc->device().updateDescriptorSets(mWrites.size(), mWrites.data(), 0, nullptr);

		return true;
	}

	DescriptorBuilder& DescriptorBuilder::updateImage(const uint32_t binding, const vk::DescriptorImageInfo* imageInfo,
	                                                  const vk::DescriptorType type)
	{
		mWrites.emplace_back(vk::WriteDescriptorSet{ {}, binding, 0, 1, type, imageInfo, nullptr });

		return *this;
	}

	DescriptorBuilder& DescriptorBuilder::updateImages(const uint32_t binding, const uint32_t imageCount,
	                                                   const vk::DescriptorImageInfo* imageInfos, const vk::DescriptorType type)
	{
		mWrites.emplace_back(vk::WriteDescriptorSet{ {}, binding, 0, imageCount, type, imageInfos, nullptr });

		return *this;
	}

	bool DescriptorBuilder::update(const vk::DescriptorSet& set)
	{
		for (vk::WriteDescriptorSet& w : mWrites)
		{
			w.dstSet = set;
		}

		mAlloc->device().updateDescriptorSets(mWrites.size(), mWrites.data(), 0, nullptr);

		return true;
	}
}
