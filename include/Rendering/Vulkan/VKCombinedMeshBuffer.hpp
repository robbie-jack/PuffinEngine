#pragma once

#include "VKRenderSystem.hpp"
#include "Assets/MeshAsset.h"
#include "Rendering/Vulkan/VKHelpers.hpp"
#include "Rendering/Vulkan/VKTypes.hpp"

namespace Puffin::Rendering::VK
{
	// Custom buffer to store vertex/index data for multiple meshes in a single large vertex/index buffer
	class CombinedMeshBuffer
	{
	public:

		void Init(std::shared_ptr<VKRenderSystem> renderer,  uint32_t vertexSize, uint32_t numVertices = 100000, uint32_t numIndices = 100000)
		{
			m_renderer = renderer;
			m_vertexSize = vertexSize;
			m_allocatedVertexCount = numVertices;
			m_allocatedIndexCount = numIndices;

			const uint32_t vertexBufferSize = m_allocatedVertexCount * m_vertexSize;

			m_vertexBuffer = Util::CreateBuffer(renderer->GetAllocator(), vertexBufferSize,
				{ vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
				vma::MemoryUsage::eAutoPreferDevice);

			const uint32_t indexBufferSize = m_allocatedIndexCount * sizeof(uint32_t);

			m_indexBuffer = Util::CreateBuffer(renderer->GetAllocator(), indexBufferSize,
				{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
				vma::MemoryUsage::eAutoPreferDevice);
		}

		bool AddMesh(std::shared_ptr<Assets::StaticMeshAsset> staticMesh);

		AllocatedBuffer& VertexBuffer() { return m_vertexBuffer; }
		AllocatedBuffer& IndexBuffer() { return m_indexBuffer; }

	private:

		struct MeshData
		{
			uint32_t vertexOffset, vertexCount;
			uint32_t indexOffset, indexCount;
		};

		std::shared_ptr<VKRenderSystem> m_renderer = nullptr;

		AllocatedBuffer m_vertexBuffer;
		AllocatedBuffer m_indexBuffer;

		uint32_t m_vertexSize = 0; // Number of bytes for each vertex

		// Number of vertices/indices currently in use
		uint32_t m_activeVertexCount = 0, m_activeIndexCount = 0;

		// Total number of vertices/indices allocated in buffers
		uint32_t m_allocatedVertexCount = 0, m_allocatedIndexCount = 0;

	};
}
