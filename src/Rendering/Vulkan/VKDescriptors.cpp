#include "Rendering/Vulkan/VKDescriptors.hpp"

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

namespace Puffin::Rendering::VK::Util
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

		vk::DescriptorPoolCreateInfo poolInfo = { createFlags, static_cast<uint32_t>(count), static_cast<uint32_t>(sizes.size()), sizes.data() };

		vk::DescriptorPool pool;
		VK_CHECK(device.createDescriptorPool(&poolInfo, nullptr, &pool));

		return pool;
	}

	void DescriptorAllocator::Init(vk::Device device)
	{
		m_device = device;
	}

	void DescriptorAllocator::ResetPools()
	{
		for (auto pool : m_usedPools)
		{
			m_device.resetDescriptorPool(pool, {});
			m_freePools.push_back(pool);
		}

		m_usedPools.clear();

		m_currentPool = nullptr;
	}

	bool DescriptorAllocator::Allocate(vk::DescriptorSet* set, vk::DescriptorSetLayout layout)
	{
		if (!m_currentPool)
		{
			m_currentPool = GrabPool();
			m_usedPools.push_back(m_currentPool);
		}

		vk::DescriptorSetAllocateInfo allocInfo = { m_currentPool, 1, &layout };

		vk::Result allocResult = m_device.allocateDescriptorSets(&allocInfo, set);
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
			m_currentPool = GrabPool();

			m_usedPools.push_back(m_currentPool);

			allocResult = m_device.allocateDescriptorSets(&allocInfo, set);

			if (allocResult == vk::Result::eSuccess)
			{
				return true;
			}
		}

		return false;
	}

	void DescriptorAllocator::Cleanup()
	{
		for (auto pool : m_freePools)
		{
			m_device.destroyDescriptorPool(pool, nullptr);
		}

		for (auto pool : m_usedPools)
		{
			m_device.destroyDescriptorPool(pool, nullptr);
		}
	}

	vk::DescriptorPool DescriptorAllocator::GrabPool()
	{
		// Check if there are reusable pools available
		if (m_freePools.size() > 0)
		{
			// Grab pool from free pools
			vk::DescriptorPool pool = m_freePools.back();
			m_freePools.pop_back();

			return pool;
		}
		else
		{
			// Create new pool
			return CreatePool(m_device, descriptorSizes, 1000, {});
		}
	}

	void DescriptorLayoutCache::Init(vk::Device device)
	{
		m_device = device;
	}

	void DescriptorLayoutCache::Cleanup()
	{
		for (auto pair : m_layoutCache)
		{
			m_device.destroyDescriptorSetLayout(pair.second, nullptr);
		}
	}

	vk::DescriptorSetLayout DescriptorLayoutCache::CreateDescriptorLayout(vk::DescriptorSetLayoutCreateInfo* info)
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
				[](vk::DescriptorSetLayoutBinding& a, vk::DescriptorSetLayoutBinding& b)
				{
					return a.binding < b.binding;
				});
		}

		// Try to grab from cache
		auto it = m_layoutCache.find(layoutInfo);
		if (it != m_layoutCache.end())
		{
			return (*it).second;
		}
		else
		{
			vk::DescriptorSetLayout layout;
			VK_CHECK(m_device.createDescriptorSetLayout(info, nullptr, &layout));

			// Add to cache
			m_layoutCache[layoutInfo] = layout;
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

	DescriptorBuilder DescriptorBuilder::Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator)
	{

	}

	DescriptorBuilder& DescriptorBuilder::BindBuffer(uint32_t binding, vk::DescriptorBufferInfo* bufferInfo,
		vk::DescriptorType type, vk::ShaderStageFlags stageFlags)
	{

	}

	DescriptorBuilder& DescriptorBuilder::BindImage(uint32_t binding, vk::DescriptorImageInfo* imageInfo,
		vk::DescriptorType type, vk::ShaderStageFlags stageFlags)
	{

	}

	DescriptorBuilder& DescriptorBuilder::BindImages(uint32_t binding, uint32_t imageCount,
		vk::DescriptorImageInfo* imageInfos, vk::DescriptorType type, vk::ShaderStageFlags stageFlags)
	{

	}

	bool DescriptorBuilder::Build(vk::DescriptorSet& set, vk::DescriptorSetLayout& layout)
	{

	}

	bool DescriptorBuilder::Build(vk::DescriptorSet& set)
	{

	}

	DescriptorBuilder& DescriptorBuilder::UpdateImage(uint32_t binding, vk::DescriptorImageInfo* imageInfo,
		vk::DescriptorType type)
	{

	}

	DescriptorBuilder& DescriptorBuilder::UpdateImages(uint32_t binding, uint32_t imageCount,
		const vk::DescriptorImageInfo* imageInfos, vk::DescriptorType type)
	{

	}

	bool DescriptorBuilder::Update(vk::DescriptorSet& set)
	{

	}
}
