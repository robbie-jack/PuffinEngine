#pragma once

#include "Assets/MeshAsset.h"

#include "Rendering/Vulkan/VKTypes.hpp"

#include <unordered_map>

namespace Puffin::Rendering::VK
{
	class VKRenderSystem;

	// Custom buffer to store vertex/index data for multiple meshes in a single large vertex/index buffer
	class CombinedMeshBuffer
	{
	public:

		void Init(std::shared_ptr<VKRenderSystem> renderer,  uint32_t vertexSize, 
			uint32_t indexSize = sizeof(uint32_t), uint32_t numVertices = 100000, uint32_t numIndices = 100000);

		void Cleanup();

		bool AddMesh(std::shared_ptr<Assets::StaticMeshAsset> staticMesh);

		AllocatedBuffer& VertexBuffer() { return m_vertexBuffer; }
		AllocatedBuffer& IndexBuffer() { return m_indexBuffer; }

	private:

		std::shared_ptr<VKRenderSystem> m_renderer = nullptr;

		struct InternalMeshData
		{
			uint32_t vertexOffset, vertexCount;
			uint32_t indexOffset, indexCount;
		};

		std::unordered_map<UUID, InternalMeshData> m_internalMeshData;

		AllocatedBuffer m_vertexBuffer;
		AllocatedBuffer m_indexBuffer;

		uint32_t m_vertexSize = 0; // Number of bytes for each vertex
		uint32_t m_indexSize = 0; // Number of bytes for each index

		// Number of vertices/indices currently in use
		uint32_t m_vertexOffset = 0, m_indexOffset = 0;

		// Total number of vertices/indices allocated in buffers
		uint32_t m_allocatedVertexCount = 0, m_allocatedIndexCount = 0;

	};
}
