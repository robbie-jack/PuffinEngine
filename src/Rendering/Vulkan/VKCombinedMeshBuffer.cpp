#include "Rendering/Vulkan/VKCombinedMeshBuffer.hpp"

#include "Rendering/Vulkan/VKRenderSystem.hpp"
#include "Rendering/Vulkan/VKHelpers.hpp"

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

namespace Puffin::Rendering::VK
{
	void CombinedMeshBuffer::Init(std::shared_ptr<VKRenderSystem> renderer, uint32_t vertexSize,
	                              uint32_t indexSize, uint32_t numVertices, uint32_t numIndices)
	{
		m_renderer = renderer;
		m_vertexSize = vertexSize;
		m_indexSize = indexSize;
		m_allocatedVertexCount = numVertices;
		m_allocatedIndexCount = numIndices;

		const uint32_t vertexBufferSize = m_allocatedVertexCount * m_vertexSize;

		m_vertexBuffer = Util::CreateBuffer(renderer->GetAllocator(), vertexBufferSize,
			{ vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice);

		const uint32_t indexBufferSize = m_allocatedIndexCount * m_indexSize;

		m_indexBuffer = Util::CreateBuffer(renderer->GetAllocator(), indexBufferSize,
			{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice);
	}

	void CombinedMeshBuffer::Cleanup()
	{
		m_vertexSize = 0;
		m_indexSize = 0;
		m_allocatedVertexCount = 0;
		m_allocatedIndexCount = 0;
		m_vertexOffset = 0;
		m_indexOffset = 0;
		m_internalMeshData.clear();

		m_renderer->GetAllocator().destroyBuffer(m_vertexBuffer.buffer, m_vertexBuffer.allocation);
		m_renderer->GetAllocator().destroyBuffer(m_indexBuffer.buffer, m_indexBuffer.allocation);
	}

	bool CombinedMeshBuffer::AddMesh(std::shared_ptr<Assets::StaticMeshAsset> staticMesh)
	{
		if (m_internalMeshData.count(staticMesh->ID()) == 1)
			return true;

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

			const uint32_t newVertexOffset = internalMeshData.vertexOffset + internalMeshData.vertexCount;
			if (newVertexOffset >= m_allocatedVertexCount)
			{
				if (!GrowVertexBuffer(newVertexOffset))
					return false;
			}

			const uint32_t newIndexOffset = internalMeshData.indexOffset + internalMeshData.indexCount;
			if (newIndexOffset >= m_allocatedIndexCount)
			{
				if (!GrowIndexBuffer(newIndexOffset))
					return false;
			}

			m_vertexOffset = newVertexOffset;
			m_indexOffset = newIndexOffset;

			const uint32_t vertexBufferSize = internalMeshData.vertexCount * m_vertexSize;
			const uint32_t indexBufferSize = internalMeshData.indexCount * m_indexSize;

			// Copy vertex data
			Util::LoadCPUDataIntoGPUBuffer(m_renderer, vk::BufferUsageFlagBits::eVertexBuffer, m_vertexBuffer.buffer, vertexBufferSize, 
				staticMesh->GetVertices().data(), 0, internalMeshData.vertexOffset * m_vertexSize);

			// Copy index data
			Util::LoadCPUDataIntoGPUBuffer(m_renderer, vk::BufferUsageFlagBits::eIndexBuffer, m_indexBuffer.buffer, indexBufferSize,
				staticMesh->GetIndices().data(), 0, internalMeshData.indexOffset * m_indexSize);

			m_internalMeshData.insert({staticMesh->ID(), internalMeshData});

			return true;
		}
		else
		{
			return false;
		}
	}

	bool CombinedMeshBuffer::GrowVertexBuffer(uint32_t minAllocationCount)
	{
		// Allocate larger vertex buffer
		const uint32_t newAllocationCount = minAllocationCount * m_bufferResizeMult;
		const uint32_t newVertexBufferSize = newAllocationCount * m_vertexSize;

		if (newAllocationCount <= m_allocatedVertexCount)
		{
			return false;
		}

		AllocatedBuffer oldVertexBuffer = m_vertexBuffer;
		m_vertexBuffer = Util::CreateBuffer(m_renderer->GetAllocator(), newVertexBufferSize,
			{ vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice);

		// Copy data from old buffer to new buffer
		const uint32_t oldVertexBufferSize = m_allocatedVertexCount * m_vertexSize;
		Util::CopyDataBetweenBuffers(m_renderer, oldVertexBuffer.buffer, m_vertexBuffer.buffer, oldVertexBufferSize);

		// Free old buffer
		m_renderer->GetAllocator().destroyBuffer(oldVertexBuffer.buffer, oldVertexBuffer.allocation);

		m_allocatedVertexCount = newAllocationCount;

		return true;
	}

	bool CombinedMeshBuffer::GrowIndexBuffer(uint32_t minAllocationCount)
	{
		// Allocated larger index buffer
		const uint32_t newAllocationCount = minAllocationCount * m_bufferResizeMult;
		const uint32_t newIndexBufferSize = newAllocationCount * m_indexSize;

		if (newAllocationCount <= m_allocatedIndexCount)
		{
			return false;
		}

		AllocatedBuffer oldIndexBuffer = m_indexBuffer;
		m_indexBuffer = Util::CreateBuffer(m_renderer->GetAllocator(), newIndexBufferSize,
			{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice);

		// Copy data from old buffer to new buffer
		const uint32_t oldIndexBufferSize = m_allocatedIndexCount * m_indexSize;
		Util::CopyDataBetweenBuffers(m_renderer, oldIndexBuffer.buffer, m_indexBuffer.buffer, oldIndexBufferSize);

		// Free old buffer
		m_renderer->GetAllocator().destroyBuffer(oldIndexBuffer.buffer, oldIndexBuffer.allocation);

		m_allocatedIndexCount = newAllocationCount;

		return true;
	}
}
