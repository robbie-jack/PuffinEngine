#include "Rendering\Vulkan\VKUnifiedGeometryBuffer.h"

#include "Rendering\Vulkan\VKRenderSystem.h"
#include "Rendering\Vulkan\VKHelpers.h"

#include <iostream>
#include <numeric>

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

namespace puffin::rendering
{
	void UnifiedGeometryBuffer::init(const std::shared_ptr<VKRenderSystem>& renderer, const uint32_t vertexSize, const uint32_t indexSize,
	                                 const vk::DeviceSize initialVertexBufferSize, const vk::DeviceSize initialIndexBufferSize, const vk::DeviceSize vertexBufferBlockSize, const vk
	                                 ::DeviceSize indexBufferBlockSize)
	{
		mRenderer = renderer;
		mVertexSize = vertexSize;
		mIndexSize = indexSize;

		mVertexBufferSize = initialVertexBufferSize;
		mIndexBufferSize = initialIndexBufferSize;

		mVertexBufferBlockSize = vertexBufferBlockSize;
		mIndexBufferBlockSize = indexBufferBlockSize;

		mMaxVertexCount = std::floor(static_cast<double>(mVertexBufferSize) / static_cast<double>(mVertexSize));
		mMaxIndexCount = std::floor(static_cast<double>(mIndexBufferSize) / static_cast<double>(mIndexSize));

		mVertexBuffer = util::createBuffer(renderer->allocator(), mVertexBufferSize,
			{ vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAuto, 
			{ vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead | 
			vma::AllocationCreateFlagBits::eMapped });

		mIndexBuffer = util::createBuffer(renderer->allocator(), mIndexBufferSize,
			{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAuto, { vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead | 
			vma::AllocationCreateFlagBits::eMapped });
	}

	void UnifiedGeometryBuffer::cleanup()
	{
		mVertexSize = 0;
		mIndexSize = 0;
		mMaxVertexCount = 0;
		mMaxIndexCount = 0;
		mVertexOffset = 0;
		mIndexOffset = 0;
		mInternalMeshData.clear();

		mRenderer->allocator().destroyBuffer(mVertexBuffer.buffer, mVertexBuffer.allocation);
		mRenderer->allocator().destroyBuffer(mIndexBuffer.buffer, mIndexBuffer.allocation);
	}

	bool UnifiedGeometryBuffer::addMesh(const std::shared_ptr<assets::StaticMeshAsset>& staticMesh)
	{
		if (mInternalMeshData.count(staticMesh->id()) == 1)
		{
			if (!mInternalMeshData[staticMesh->id()].isActive)
			{
				mInternalMeshData[staticMesh->id()].isActive = true;

				mActiveVertexCount += mInternalMeshData[staticMesh->id()].vertexCount;
				mActiveIndexCount += mInternalMeshData[staticMesh->id()].indexCount;
			}

			return true;
		}
		else
		{
			if (staticMesh && staticMesh->load())
			{
				if (mVertexSize != staticMesh->vertexSize() || mIndexSize != staticMesh->indexSize())
					return false;

				// Init mesh offset data
				InternalMeshData internalMeshData;
				internalMeshData.vertexOffset = mVertexOffset;
				internalMeshData.indexOffset = mIndexOffset;
				internalMeshData.vertexCount = staticMesh->numVertices();
				internalMeshData.indexCount = staticMesh->numIndices();
				internalMeshData.isActive = true;

				const uint32_t newVertexOffset = internalMeshData.vertexOffset + internalMeshData.vertexCount;
				if (newVertexOffset >= mMaxVertexCount)
				{
					if (!growVertexBuffer(newVertexOffset))
						return false;
				}

				const uint32_t newIndexOffset = internalMeshData.indexOffset + internalMeshData.indexCount;
				if (newIndexOffset >= mMaxIndexCount)
				{
					if (!growIndexBuffer(newIndexOffset))
						return false;
				}

				mVertexOffset = newVertexOffset;
				mIndexOffset = newIndexOffset;

				const uint32_t vertexBufferSize = internalMeshData.vertexCount * mVertexSize;
				const uint32_t indexBufferSize = internalMeshData.indexCount * mIndexSize;

				// Copy vertex data
				util::loadCpuDataIntoGPUBuffer(mRenderer, vk::BufferUsageFlagBits::eVertexBuffer, mVertexBuffer, vertexBufferSize,
					staticMesh->vertices().data(), 0, internalMeshData.vertexOffset * mVertexSize);

				// Copy index data
				util::loadCpuDataIntoGPUBuffer(mRenderer, vk::BufferUsageFlagBits::eIndexBuffer, mIndexBuffer, indexBufferSize,
					staticMesh->indices().data(), 0, internalMeshData.indexOffset * mIndexSize);

				mInternalMeshData.insert({ staticMesh->id(), internalMeshData });

				mActiveVertexCount += internalMeshData.vertexCount;
				mActiveIndexCount += internalMeshData.indexCount;

				return true;
			}
			else
			{
				return false;
			}
		}
	}

	bool UnifiedGeometryBuffer::removeMeshes(const std::set<PuffinId>& staticMeshesToRemove)
	{
		if (staticMeshesToRemove.empty())
			return true;

		for (const auto& meshId : staticMeshesToRemove)
		{
			if (mInternalMeshData[meshId].isActive)
			{
				mInternalMeshData[meshId].isActive = false;

				mActiveVertexCount -= mInternalMeshData[meshId].vertexCount;
				mActiveIndexCount -= mInternalMeshData[meshId].vertexCount;
			}
		}

		// Shrink vertex buffer when half of allocated vertices are inactive
		const double activeVerticesRatio = mActiveVertexCount / mVertexOffset;
		if (activeVerticesRatio <= mShrinkUsageThreshold)
		{
			shrinkVertexBuffer(mActiveVertexCount);
		}

		// Shrink index buffer when half of allocated vertices are inactive
		const double activeIndicesRatio = mActiveIndexCount / mIndexOffset;
		if (activeIndicesRatio <= mShrinkUsageThreshold)
		{
			shrinkIndexBuffer(mActiveIndexCount);
		}

		return true;
	}

	bool UnifiedGeometryBuffer::growVertexBuffer(const uint32_t minVertexCount)
	{
		// Allocate larger vertex buffer
		const vk::DeviceSize minVertexBufferSize = minVertexCount * mVertexSize;

		// Find new buffer size which can fit all vertices

		vk::DeviceSize newVertexBufferSize = mVertexBufferSize;
		while (newVertexBufferSize < minVertexBufferSize)
		{
			newVertexBufferSize += mVertexBufferBlockSize;
		}

		const AllocatedBuffer oldVertexBuffer = mVertexBuffer;
		mVertexBuffer = util::createBuffer(mRenderer->allocator(), newVertexBufferSize,
			{ vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice);

		// Copy data from old buffer to new buffer
		const vk::DeviceSize oldVertexBufferSize = mVertexOffset * mVertexSize;
		util::copyDataBetweenBuffers(mRenderer, oldVertexBuffer.buffer, mVertexBuffer.buffer, oldVertexBufferSize);

		// Free old buffer
		mRenderer->allocator().destroyBuffer(oldVertexBuffer.buffer, oldVertexBuffer.allocation);

		mVertexBufferSize = newVertexBufferSize;

		mMaxVertexCount = std::floor(static_cast<double>(mVertexBufferSize) / static_cast<double>(mVertexSize));;

		return true;
	}

	bool UnifiedGeometryBuffer::growIndexBuffer(const uint32_t minIndexCount)
	{
		// Allocate larger index buffer
		const uint32_t minIndexBufferSize = minIndexCount * mIndexSize;

		// Find new buffer size which can fit all indices

		vk::DeviceSize newIndexBufferSize = mIndexBufferSize;
		while (newIndexBufferSize < minIndexBufferSize)
		{
			newIndexBufferSize += mIndexBufferBlockSize;
		}

		const AllocatedBuffer oldIndexBuffer = mIndexBuffer;
		mIndexBuffer = util::createBuffer(mRenderer->allocator(), newIndexBufferSize,
			{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice);

		// Copy data from old buffer to new buffer
		const vk::DeviceSize oldIndexBufferSize = mIndexOffset * mIndexSize;
		util::copyDataBetweenBuffers(mRenderer, oldIndexBuffer.buffer, mIndexBuffer.buffer, oldIndexBufferSize);

		// Free old buffer
		mRenderer->allocator().destroyBuffer(oldIndexBuffer.buffer, oldIndexBuffer.allocation);

		mIndexBufferSize = newIndexBufferSize;

		mMaxIndexCount = std::floor(static_cast<double>(mIndexBufferSize) / static_cast<double>(mIndexSize));

		return true;
	}

	bool UnifiedGeometryBuffer::shrinkVertexBuffer(uint32_t minVertexCount)
	{


		return true;
	}

	bool UnifiedGeometryBuffer::shrinkIndexBuffer(uint32_t minVertexCount)
	{
		return true;
	}
}
