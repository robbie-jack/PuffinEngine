#include <Rendering/VKDescriptors.h>

// STL
#include <algorithm>

namespace Puffin
{
	namespace Rendering
	{
		namespace VKUtil
		{
			VkDescriptorPool CreatePool(VkDevice device, const DescriptorAllocator::PoolSizes& poolSizes, int count, VkDescriptorPoolCreateFlags flags)
			{
				// Grab all pool sizes from PoolSizes struct
				std::vector<VkDescriptorPoolSize> sizes;
				sizes.reserve(poolSizes.sizes.size());

				for (auto sz : poolSizes.sizes)
				{
					sizes.push_back({ sz.first, uint32_t(sz.second * count) });
				}

				// Create Descriptor Pool with sizes and return
				VkDescriptorPoolCreateInfo poolInfo = {};
				poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				poolInfo.flags = flags;
				poolInfo.maxSets = count;
				poolInfo.poolSizeCount = (uint32_t)sizes.size();
				poolInfo.pPoolSizes = sizes.data();

				VkDescriptorPool descriptorPool;
				vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);

				return descriptorPool;
			}

			//////////////////////////////////////////////////
			// Descriptor Allocator
			//////////////////////////////////////////////////

			void DescriptorAllocator::Init(VkDevice newDevice)
			{
				device = newDevice;
			}

			bool DescriptorAllocator::Allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout)
			{
				// Initialize currentPool handle if its null
				if (currentPool == VK_NULL_HANDLE)
				{
					currentPool = GrabPool();
					usedPools.push_back(currentPool);
				}

				VkDescriptorSetAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.pNext = nullptr;

				allocInfo.pSetLayouts = &layout;
				allocInfo.descriptorPool = currentPool;
				allocInfo.descriptorSetCount = 1;

				// Try to allocate descriptor set
				VkResult allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);
				bool needReallocate = false;

				switch (allocResult)
				{
				case VK_SUCCESS:
					// No problems
					return true;
				case VK_ERROR_FRAGMENTED_POOL:
				case VK_ERROR_OUT_OF_POOL_MEMORY:
					// Need to reallocate pool
					needReallocate = true;
					break;
				default:
					// Unrecoverable error
					return false;
				}

				if (needReallocate)
				{
					// Allocate new pool and retry
					currentPool = GrabPool();
					usedPools.push_back(currentPool);

					allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);

					// If its stil fails then we have big issues
					if (allocResult == VK_SUCCESS)
						return true;
				}

				return false;
			}

			void DescriptorAllocator::ResetPools()
			{
				// Reset every pool
				for (auto p : usedPools)
				{
					vkResetDescriptorPool(device, p, 0);
				}

				// Move all pools to reusable vector
				freePools = usedPools;
				usedPools.clear();

				// Reset current pool handle back to null
				currentPool = VK_NULL_HANDLE;
			}

			VkDescriptorPool DescriptorAllocator::GrabPool()
			{
				// There are reusable pools available
				if (freePools.size() > 0)
				{
					// Grab pool from back of vector
					VkDescriptorPool pool = freePools.back();
					freePools.pop_back();
					return pool;
				}
				else
				{
					// No pool available, so create new one
					return CreatePool(device, descriptorSizes, 1000, 0);
				}
			}

			void DescriptorAllocator::Cleanup()
			{
				// Delete every pool held
				for (auto p : freePools)
				{
					vkDestroyDescriptorPool(device, p, nullptr);
				}

				for (auto p : usedPools)
				{
					vkDestroyDescriptorPool(device, p, nullptr);
				}
			}

			//////////////////////////////////////////////////
			// Descriptor Layout Cache
			//////////////////////////////////////////////////

			void DescriptorLayoutCache::Init(VkDevice newDevice)
			{
				device = newDevice;
			}

			VkDescriptorSetLayout DescriptorLayoutCache::CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo* info)
			{
				DescriptorLayoutInfo layoutInfo;
				layoutInfo.bindings.reserve(info->bindingCount);

				bool isSorted = true;
				int lastBinding = 1;

				// Copy from direct info struct into our own one
				for (int i = 0; i < info->bindingCount; i++)
				{
					layoutInfo.bindings.push_back(info->pBindings[i]);

					// Check that bindings are in strict order
					if (info->pBindings[i].binding > lastBinding)
					{
						lastBinding = info->pBindings[i].binding;
					}
					else
					{
						isSorted = false;
					}
				}

				// Sort bindings if not in order
				if (!isSorted)
				{
					std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(),
						[](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b)
					{
						return a.binding < b.binding;
					});
				}

				// Try to grab from cache
				auto it = layoutCache.find(layoutInfo);
				if (it != layoutCache.end())
				{
					return (*it).second;
				}
				else
				{
					// Create a new one if not found
					VkDescriptorSetLayout layout;
					vkCreateDescriptorSetLayout(device, info, nullptr, &layout);

					// Add to cache
					layoutCache[layoutInfo] = layout;
					return layout;
				}
			}

			void DescriptorLayoutCache::Cleanup()
			{
				// Delete every descriptor layout held
				for (auto pair : layoutCache)
				{
					vkDestroyDescriptorSetLayout(device, pair.second, nullptr);
				}
			}

			//////////////////////////////////////////////////
			// Descriptor Layout Info
			//////////////////////////////////////////////////

			bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
			{
				if (other.bindings.size() != bindings.size())
				{
					return false;
				}
				else
				{
					// Compare each of the bindings is the same. Bindings are sorted so they will match
					for (int i = 0; i < bindings.size(); i++)
					{
						if (other.bindings[i].binding != bindings[i].binding)
						{
							return false;
						}
						if (other.bindings[i].descriptorType != bindings[i].descriptorType)
						{
							return false;
						}
						if (other.bindings[i].descriptorCount != bindings[i].descriptorCount)
						{
							return false;
						}
						if (other.bindings[i].stageFlags != bindings[i].stageFlags)
						{
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
					// Pack binding data into single int64. Not fully correct but its ok
					size_t bindingHash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

					// Shuffle packed binding data and xor it with the main hash
					result ^= hash<size_t>()(bindingHash);
				}

				return result;
			}

			//////////////////////////////////////////////////
			// Descriptor builder
			//////////////////////////////////////////////////

			DescriptorBuilder DescriptorBuilder::Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator)
			{
				DescriptorBuilder builder;

				builder.cache = layoutCache;
				builder.alloc = allocator;
				return builder;
			}

			DescriptorBuilder& DescriptorBuilder::BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo,
				VkDescriptorType type, VkShaderStageFlags stageFlags)
			{
				// Create descriptor binding for layout
				VkDescriptorSetLayoutBinding newBinding{};

				newBinding.descriptorCount = 1;
				newBinding.descriptorType = type;
				newBinding.pImmutableSamplers = nullptr;
				newBinding.stageFlags = stageFlags;
				newBinding.binding = binding;

				bindings.push_back(newBinding);

				// Create descriptor write
				VkWriteDescriptorSet newWrite{};
				newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				newWrite.pNext = nullptr;

				newWrite.descriptorCount = 1;
				newWrite.descriptorType = type;
				newWrite.pBufferInfo = bufferInfo;
				newWrite.dstBinding = binding;

				writes.push_back(newWrite);
				return *this;
			}

			DescriptorBuilder& DescriptorBuilder::BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo,
				VkDescriptorType type, VkShaderStageFlags stageFlags)
			{
				// Create descriptor binding for layout
				VkDescriptorSetLayoutBinding newBinding{};

				newBinding.descriptorCount = 1;
				newBinding.descriptorType = type;
				newBinding.pImmutableSamplers = nullptr;
				newBinding.stageFlags = stageFlags;
				newBinding.binding = binding;

				bindings.push_back(newBinding);

				// Create descriptor write
				VkWriteDescriptorSet newWrite{};
				newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				newWrite.pNext = nullptr;

				newWrite.descriptorCount = 1;
				newWrite.descriptorType = type;
				newWrite.pImageInfo = imageInfo;
				newWrite.dstBinding = binding;

				writes.push_back(newWrite);
				return *this;
			}

			DescriptorBuilder& DescriptorBuilder::BindImages(uint32_t binding, std::vector<VkDescriptorImageInfo>& imageInfos,
				VkDescriptorType type, VkShaderStageFlags stageFlags)
			{
				// Create descriptor binding for layout
				VkDescriptorSetLayoutBinding newBinding{};

				newBinding.descriptorCount = static_cast<uint32_t>(imageInfos.size());
				newBinding.descriptorType = type;
				newBinding.pImmutableSamplers = nullptr;
				newBinding.stageFlags = stageFlags;
				newBinding.binding = binding;

				bindings.push_back(newBinding);

				// Create descriptor write
				VkWriteDescriptorSet newWrite{};
				newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				newWrite.pNext = nullptr;

				newWrite.descriptorCount = static_cast<uint32_t>(imageInfos.size());
				newWrite.descriptorType = type;
				newWrite.pImageInfo = imageInfos.data();
				newWrite.dstBinding = binding;
				newWrite.dstArrayElement = 0;
				newWrite.pBufferInfo = 0;

				writes.push_back(newWrite);
				return *this;
			}

			bool DescriptorBuilder::Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout)
			{
				// Build Layout First
				VkDescriptorSetLayoutCreateInfo layoutInfo{};
				layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layoutInfo.pNext = nullptr;

				layoutInfo.pBindings = bindings.data();
				layoutInfo.bindingCount = bindings.size();

				layout = cache->CreateDescriptorSetLayout(&layoutInfo);

				// Allocate Descriptor
				bool success = alloc->Allocate(&set, layout);
				if (!success) { return false; };

				// Write Descriptor
				for (VkWriteDescriptorSet& w : writes)
				{
					w.dstSet = set;
				}

				vkUpdateDescriptorSets(alloc->device, writes.size(), writes.data(), 0, nullptr);

				return true;
			}

			bool DescriptorBuilder::Build(VkDescriptorSet& set)
			{
				VkDescriptorSetLayout layout;
				return Build(set, layout);
			}
		}
	}
}