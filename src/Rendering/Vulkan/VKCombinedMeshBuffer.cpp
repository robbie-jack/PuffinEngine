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
				return false;

			const uint32_t newIndexOffset = internalMeshData.indexOffset + internalMeshData.indexCount;
			if (newIndexOffset >= m_allocatedIndexCount)
				return false;

			m_vertexOffset = newVertexOffset;
			m_indexOffset = newIndexOffset;

			const uint32_t vertexBufferSize = internalMeshData.vertexCount * m_vertexSize;
			const uint32_t indexBufferSize = internalMeshData.indexCount * m_indexSize;

			// Copy vertex data
			Util::LoadCPUDataIntoGPUBuffer(m_renderer, vk::BufferUsageFlagBits::eVertexBuffer, m_vertexBuffer.buffer, vertexBufferSize, 
				staticMesh->GetVertices().data(), 0, internalMeshData.vertexOffset * m_vertexSize);

			// Copy index data
			Util::LoadCPUDataIntoGPUBuffer(m_renderer, vk::BufferUsageFlagBits::eVertexBuffer, m_indexBuffer.buffer, indexBufferSize,
				staticMesh->GetVertices().data(), 0, internalMeshData.indexOffset * m_indexSize);

			m_internalMeshData.insert({staticMesh->ID(), internalMeshData});

			return true;
		}
		else
		{
			return false;
		}
	}
}
