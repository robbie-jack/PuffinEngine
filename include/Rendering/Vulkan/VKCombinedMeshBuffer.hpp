#pragma once

#include "Assets/MeshAsset.h"

#include "Rendering/Vulkan/VKTypes.hpp"

#include <unordered_map>
#include <vector>
#include <set>

namespace Puffin::Rendering::VK
{
	class VKRenderSystem;

	// Custom buffer to store vertex/index data for multiple meshes in a single large vertex/index buffer
	class CombinedMeshBuffer
	{
	public:

		void Init(std::shared_ptr<VKRenderSystem> renderer, uint32_t vertexSize,
		          uint32_t indexSize = sizeof(uint32_t), uint32_t numVertices = 500000, uint32_t numIndices = 1000000);

		void Cleanup();

		bool AddMesh(std::shared_ptr<Assets::StaticMeshAsset> staticMesh);

		bool HasMesh(UUID staticMeshID) const
		{
			return m_internalMeshData.count(staticMeshID) == 1;
		}

		bool RemoveMeshes(const std::set<UUID>& staticMeshesToRemove);

		inline uint32_t MeshVertexOffset(UUID meshID) { return m_internalMeshData[meshID].vertexOffset; }
		inline uint32_t MeshIndexOffset(UUID meshID) { return m_internalMeshData[meshID].indexOffset; }

		inline uint32_t MeshVertexCount(UUID meshID) { return m_internalMeshData[meshID].vertexCount; }
		inline uint32_t MeshIndexCount(UUID meshID) { return m_internalMeshData[meshID].indexCount; }

		AllocatedBuffer& VertexBuffer() { return m_vertexBuffer; }
		AllocatedBuffer& IndexBuffer() { return m_indexBuffer; }

		inline void SetBufferGrowMult(float inMult) { m_bufferGrowMult = inMult; }
		inline void SetBufferShrinkMult(float inMult) { m_bufferShrinkMult = inMult; }
		inline void SetBufferShrinkThreshold(float inThreshold) { m_bufferShrinkThreshold = inThreshold; }

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

		float m_bufferGrowMult = 1.5f; // How much more space than minimum count to allocate when buffer grows
		float m_bufferShrinkMult = 1.2f; // How much more space than minimum count to allocate when buffer shrinks
		float m_bufferShrinkThreshold = 0.5f; // How much of the buffer is use before it should be shrunk

		bool UpdateVertexBuffer(uint32_t vertexCount);
		bool UpdateIndexBuffer(uint32_t indexCount);
	};
}
