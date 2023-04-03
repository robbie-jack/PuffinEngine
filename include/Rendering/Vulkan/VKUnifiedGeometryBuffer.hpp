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
	class UnifiedGeometryBuffer
	{
	public:

		void Init(std::shared_ptr<VKRenderSystem> renderer, uint32_t vertexSize, uint32_t indexSize = sizeof(uint32_t), 
			vk::DeviceSize defaultVertexBufferSize = 1 * 1024 * 1024, vk::DeviceSize defaultIndexBufferSize = 1 * 1024 * 1024);

		void Cleanup();

		bool AddMesh(std::shared_ptr<Assets::StaticMeshAsset> staticMesh);

		bool HasMesh(UUID staticMeshID) const
		{
			return m_internalMeshData.count(staticMeshID) == 1;
		}

		// CURRENTLY BROKEN, DO NOT USE!!!!!
		bool RemoveMeshes(const std::set<UUID>& staticMeshesToRemove);

		inline uint32_t MeshVertexOffset(UUID meshID) { return m_internalMeshData[meshID].vertexOffset; }
		inline uint32_t MeshIndexOffset(UUID meshID) { return m_internalMeshData[meshID].indexOffset; }

		inline uint32_t MeshVertexCount(UUID meshID) { return m_internalMeshData[meshID].vertexCount; }
		inline uint32_t MeshIndexCount(UUID meshID) { return m_internalMeshData[meshID].indexCount; }

		AllocatedBuffer& VertexBuffer() { return m_vertexBuffer; }
		AllocatedBuffer& IndexBuffer() { return m_indexBuffer; }

	private:

		std::shared_ptr<VKRenderSystem> m_renderer = nullptr;

		struct InternalMeshData
		{
			uint32_t vertexOffset, vertexCount;
			uint32_t indexOffset, indexCount;
			bool isActive;
		};

		std::unordered_map<UUID, InternalMeshData> m_internalMeshData;

		AllocatedBuffer m_vertexBuffer;
		AllocatedBuffer m_indexBuffer;

		uint32_t m_vertexSize = 0; // Number of bytes for each vertex
		uint32_t m_indexSize = 0; // Number of bytes for each index

		// Offset into buffers where vertices/indices have been added
		uint32_t m_vertexOffset = 0, m_indexOffset = 0;

		// Number of vertices/indices in use
		uint32_t m_activeVertexCount = 0, m_activeIndexCount = 0;

		// Total number of vertices/indices allocated in buffers
		uint32_t m_maxVertexCount = 0, m_maxIndexCount = 0;

		vk::DeviceSize m_vertexBufferSize = 0, m_indexBufferSize = 0;
		vk::DeviceSize m_vertexBufferSizeBlock = 64 * 1024 * 1024;
		vk::DeviceSize m_indexBufferSizeBlock = 64 * 1024 * 1024;

		// How much of buffer is unused before it should be shrank
		double m_shrinkUsageThreshold = 0.5f;

		bool GrowVertexBuffer(uint32_t minVertexCount);
		bool GrowIndexBuffer(uint32_t minIndexCount);

		bool ShrinkVertexBuffer(uint32_t minVertexCount);
		bool ShrinkIndexBuffer(uint32_t minVertexCount);
	};
}
