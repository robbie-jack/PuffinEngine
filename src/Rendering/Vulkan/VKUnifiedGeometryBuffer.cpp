#include "Rendering/Vulkan/VKUnifiedGeometryBuffer.hpp"

#include "Rendering/Vulkan/VKRenderSystem.hpp"
#include "Rendering/Vulkan/VKHelpers.hpp"

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

namespace puffin::Rendering::VK
{
	void UnifiedGeometryBuffer::Init(std::shared_ptr<VKRenderSystem> renderer, uint32_t vertexSize, uint32_t indexSize,
	                                 vk::DeviceSize initialVertexBufferSize, vk::DeviceSize initialIndexBufferSize, vk::DeviceSize vertexBufferBlockSize, vk
	                                 ::DeviceSize indexBufferBlockSize)
	{
		m_renderer = renderer;
		m_vertexSize = vertexSize;
		m_indexSize = indexSize;

		m_vertexBufferSize = initialVertexBufferSize;
		m_indexBufferSize = initialIndexBufferSize;

		m_vertexBufferBlockSize = vertexBufferBlockSize;
		m_indexBufferBlockSize = indexBufferBlockSize;

		m_maxVertexCount = std::floor(static_cast<double>(m_vertexBufferSize) / static_cast<double>(m_vertexSize));
		m_maxIndexCount = std::floor(static_cast<double>(m_indexBufferSize) / static_cast<double>(m_indexSize));

		m_vertexBuffer = Util::CreateBuffer(renderer->GetAllocator(), m_vertexBufferSize,
			{ vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAuto, 
			{ vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead | 
			vma::AllocationCreateFlagBits::eMapped });

		m_indexBuffer = Util::CreateBuffer(renderer->GetAllocator(), m_indexBufferSize,
			{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAuto, { vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead | 
			vma::AllocationCreateFlagBits::eMapped });
	}

	void UnifiedGeometryBuffer::Cleanup()
	{
		m_vertexSize = 0;
		m_indexSize = 0;
		m_maxVertexCount = 0;
		m_maxIndexCount = 0;
		m_vertexOffset = 0;
		m_indexOffset = 0;
		m_internalMeshData.clear();

		m_renderer->GetAllocator().destroyBuffer(m_vertexBuffer.buffer, m_vertexBuffer.allocation);
		m_renderer->GetAllocator().destroyBuffer(m_indexBuffer.buffer, m_indexBuffer.allocation);
	}

	bool UnifiedGeometryBuffer::AddMesh(std::shared_ptr<Assets::StaticMeshAsset> staticMesh)
	{
		if (m_internalMeshData.count(staticMesh->ID()) == 1)
		{
			if (!m_internalMeshData[staticMesh->ID()].isActive)
			{
				m_internalMeshData[staticMesh->ID()].isActive = true;

				m_activeVertexCount += m_internalMeshData[staticMesh->ID()].vertexCount;
				m_activeIndexCount += m_internalMeshData[staticMesh->ID()].indexCount;
			}

			return true;
		}
		else
		{
			if (staticMesh && staticMesh->Load())
			{
				if (m_vertexSize != staticMesh->GetVertexSize() || m_indexSize != staticMesh->GetIndexSize())
					return false;

				// Init mesh offset data
				InternalMeshData internalMeshData;
				internalMeshData.vertexOffset = m_vertexOffset;
				internalMeshData.indexOffset = m_indexOffset;
				internalMeshData.vertexCount = staticMesh->GetNumVertices();
				internalMeshData.indexCount = staticMesh->GetNumIndices();
				internalMeshData.isActive = true;

				const uint32_t newVertexOffset = internalMeshData.vertexOffset + internalMeshData.vertexCount;
				if (newVertexOffset >= m_maxVertexCount)
				{
					if (!GrowVertexBuffer(newVertexOffset))
						return false;
				}

				const uint32_t newIndexOffset = internalMeshData.indexOffset + internalMeshData.indexCount;
				if (newIndexOffset >= m_maxIndexCount)
				{
					if (!GrowIndexBuffer(newIndexOffset))
						return false;
				}

				m_vertexOffset = newVertexOffset;
				m_indexOffset = newIndexOffset;

				const uint32_t vertexBufferSize = internalMeshData.vertexCount * m_vertexSize;
				const uint32_t indexBufferSize = internalMeshData.indexCount * m_indexSize;

				// Copy vertex data
				Util::LoadCPUDataIntoGPUBuffer(m_renderer, vk::BufferUsageFlagBits::eVertexBuffer, m_vertexBuffer, vertexBufferSize,
					staticMesh->GetVertices().data(), 0, internalMeshData.vertexOffset * m_vertexSize);

				// Copy index data
				Util::LoadCPUDataIntoGPUBuffer(m_renderer, vk::BufferUsageFlagBits::eIndexBuffer, m_indexBuffer, indexBufferSize,
					staticMesh->GetIndices().data(), 0, internalMeshData.indexOffset * m_indexSize);

				m_internalMeshData.insert({ staticMesh->ID(), internalMeshData });

				m_activeVertexCount += internalMeshData.vertexCount;
				m_activeIndexCount += internalMeshData.indexCount;

				return true;
			}
			else
			{
				return false;
			}
		}
	}

	bool UnifiedGeometryBuffer::RemoveMeshes(const std::set<UUID>& staticMeshesToRemove)
	{
		if (staticMeshesToRemove.size() == 0)
			return true;

		for (const auto& meshID : staticMeshesToRemove)
		{
			if (m_internalMeshData[meshID].isActive)
			{
				m_internalMeshData[meshID].isActive = false;

				m_activeVertexCount -= m_internalMeshData[meshID].vertexCount;
				m_activeIndexCount -= m_internalMeshData[meshID].vertexCount;
			}
		}

		// Shrink vertex buffer when half of allocated vertices are inactive
		const double activeVerticesRatio = m_activeVertexCount / m_vertexOffset;
		if (activeVerticesRatio <= m_shrinkUsageThreshold)
		{
			ShrinkVertexBuffer(m_activeVertexCount);
		}

		// Shrink index buffer when half of allocated vertices are inactive
		const double activeIndicesRatio = m_activeIndexCount / m_indexOffset;
		if (activeIndicesRatio <= m_shrinkUsageThreshold)
		{
			ShrinkIndexBuffer(m_activeIndexCount);
		}

		return true;
	}

	bool UnifiedGeometryBuffer::GrowVertexBuffer(uint32_t minVertexCount)
	{
		// Allocate larger vertex buffer
		const vk::DeviceSize minVertexBufferSize = minVertexCount * m_vertexSize;

		// Find new buffer size which can fit all vertices

		vk::DeviceSize newVertexBufferSize = m_vertexBufferSize;
		while (newVertexBufferSize < minVertexBufferSize)
		{
			newVertexBufferSize += m_vertexBufferBlockSize;
		}

		AllocatedBuffer oldVertexBuffer = m_vertexBuffer;
		m_vertexBuffer = Util::CreateBuffer(m_renderer->GetAllocator(), newVertexBufferSize,
			{ vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice);

		// Copy data from old buffer to new buffer
		const vk::DeviceSize oldVertexBufferSize = m_vertexOffset * m_vertexSize;
		Util::CopyDataBetweenBuffers(m_renderer, oldVertexBuffer.buffer, m_vertexBuffer.buffer, oldVertexBufferSize);

		// Free old buffer
		m_renderer->GetAllocator().destroyBuffer(oldVertexBuffer.buffer, oldVertexBuffer.allocation);

		m_vertexBufferSize = newVertexBufferSize;

		m_maxVertexCount = std::floor(static_cast<double>(m_vertexBufferSize) / static_cast<double>(m_vertexSize));;

		return true;
	}

	bool UnifiedGeometryBuffer::GrowIndexBuffer(uint32_t minIndexCount)
	{
		// Allocate larger index buffer
		const uint32_t minIndexBufferSize = minIndexCount * m_indexSize;

		// Find new buffer size which can fit all indices

		vk::DeviceSize newIndexBufferSize = m_indexBufferSize;
		while (newIndexBufferSize < minIndexBufferSize)
		{
			newIndexBufferSize += m_indexBufferBlockSize;
		}

		AllocatedBuffer oldIndexBuffer = m_indexBuffer;
		m_indexBuffer = Util::CreateBuffer(m_renderer->GetAllocator(), newIndexBufferSize,
			{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice);

		// Copy data from old buffer to new buffer
		const vk::DeviceSize oldIndexBufferSize = m_indexOffset * m_indexSize;
		Util::CopyDataBetweenBuffers(m_renderer, oldIndexBuffer.buffer, m_indexBuffer.buffer, oldIndexBufferSize);

		// Free old buffer
		m_renderer->GetAllocator().destroyBuffer(oldIndexBuffer.buffer, oldIndexBuffer.allocation);

		m_indexBufferSize = newIndexBufferSize;

		m_maxIndexCount = std::floor(static_cast<double>(m_indexBufferSize) / static_cast<double>(m_indexSize));

		return true;
	}

	bool UnifiedGeometryBuffer::ShrinkVertexBuffer(uint32_t minVertexCount)
	{


		return true;
	}

	bool UnifiedGeometryBuffer::ShrinkIndexBuffer(uint32_t minVertexCount)
	{
		return true;
	}
}
