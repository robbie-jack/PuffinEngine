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
				if (!UpdateVertexBuffer(newVertexOffset * m_bufferGrowMult))
					return false;
			}

			const uint32_t newIndexOffset = internalMeshData.indexOffset + internalMeshData.indexCount;
			if (newIndexOffset >= m_allocatedIndexCount)
			{
				if (!UpdateIndexBuffer(newIndexOffset * m_bufferGrowMult))
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

	bool CombinedMeshBuffer::RemoveMeshes(const std::set<UUID>& staticMeshesToRemove)
	{
		if (staticMeshesToRemove.size() == 0)
			return true;

		// Remove inactive mesh data
		m_vertexOffset = 0;
		m_indexOffset = 0;

		// Allocate new vertex/index buffer for un-removed data
		const uint32_t vertexBufferSize = m_allocatedVertexCount * m_vertexSize;
		AllocatedBuffer oldVertexBuffer = m_vertexBuffer;
		m_vertexBuffer = Util::CreateBuffer(m_renderer->GetAllocator(), vertexBufferSize,
			{ vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice);

		const uint32_t indexBufferSize = m_allocatedIndexCount * m_indexSize;
		AllocatedBuffer oldIndexBuffer = m_indexBuffer;
		m_indexBuffer = Util::CreateBuffer(m_renderer->GetAllocator(), indexBufferSize,
			{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice);

		// Copy all still active meshes into new buffer
		for (auto& [fst, snd] : m_internalMeshData)
		{
			if (staticMeshesToRemove.count(fst) == 1)
				continue;

			// Copy Vertex Data
			const uint32_t vertexCopySize = snd.vertexCount * m_vertexSize;
			Util::CopyDataBetweenBuffers(m_renderer, oldVertexBuffer.buffer, m_vertexBuffer.buffer,
				vertexCopySize, snd.vertexOffset, m_vertexOffset);

			// Copy Index Data
			const uint32_t indexCopySize = snd.indexCount * m_indexSize;
			Util::CopyDataBetweenBuffers(m_renderer, oldIndexBuffer.buffer, m_indexBuffer.buffer,
				indexCopySize, snd.indexOffset, m_indexOffset);

			snd.vertexOffset = m_vertexOffset;
			snd.indexOffset = m_indexOffset;

			m_vertexOffset += snd.vertexCount;
			m_indexOffset += snd.indexCount;
		}

		m_renderer->GetAllocator().destroyBuffer(oldVertexBuffer.buffer, oldVertexBuffer.allocation);
		m_renderer->GetAllocator().destroyBuffer(oldIndexBuffer.buffer, oldIndexBuffer.allocation);

		for (const auto& staticMeshID : staticMeshesToRemove)
		{
			m_internalMeshData.erase(staticMeshID);
		}

		if (m_vertexOffset < m_allocatedVertexCount * m_bufferShrinkThreshold)
		{
			UpdateVertexBuffer(m_vertexOffset * m_bufferShrinkMult);
		}

		if (m_indexOffset < m_allocatedIndexCount * m_bufferShrinkThreshold)
		{
			UpdateIndexBuffer(m_indexOffset * m_bufferShrinkMult);
		}

		return true;
	}

	bool CombinedMeshBuffer::UpdateVertexBuffer(uint32_t vertexCount)
	{
		// Allocate larger vertex buffer
		const uint32_t newVertexBufferSize = vertexCount * m_vertexSize;

		AllocatedBuffer oldVertexBuffer = m_vertexBuffer;
		m_vertexBuffer = Util::CreateBuffer(m_renderer->GetAllocator(), newVertexBufferSize,
			{ vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice);

		// Copy data from old buffer to new buffer
		const uint32_t oldVertexBufferSize = m_vertexOffset * m_vertexSize;
		Util::CopyDataBetweenBuffers(m_renderer, oldVertexBuffer.buffer, m_vertexBuffer.buffer, oldVertexBufferSize);

		// Free old buffer
		m_renderer->GetAllocator().destroyBuffer(oldVertexBuffer.buffer, oldVertexBuffer.allocation);

		m_allocatedVertexCount = vertexCount;

		return true;
	}

	bool CombinedMeshBuffer::UpdateIndexBuffer(uint32_t indexCount)
	{
		// Allocate larger index buffer
		const uint32_t newIndexBufferSize = indexCount * m_indexSize;

		AllocatedBuffer oldIndexBuffer = m_indexBuffer;
		m_indexBuffer = Util::CreateBuffer(m_renderer->GetAllocator(), newIndexBufferSize,
			{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice);

		// Copy data from old buffer to new buffer
		const uint32_t oldIndexBufferSize = m_indexOffset * m_indexSize;
		Util::CopyDataBetweenBuffers(m_renderer, oldIndexBuffer.buffer, m_indexBuffer.buffer, oldIndexBufferSize);

		// Free old buffer
		m_renderer->GetAllocator().destroyBuffer(oldIndexBuffer.buffer, oldIndexBuffer.allocation);

		m_allocatedIndexCount = indexCount;

		return true;
	}
}
