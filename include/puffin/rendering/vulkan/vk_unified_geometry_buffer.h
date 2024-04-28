#pragma once

#include <unordered_map>
#include <vector>
#include <set>

#include "puffin/rendering/vulkan/vk_types.h"
#include "puffin/assets/mesh_asset.h"

namespace puffin::rendering
{
	class VKRenderSystem;

	// Custom buffer to store vertex/index data for multiple meshes in a single large vertex/index buffer
	class UnifiedGeometryBuffer
	{
	public:

		void init(const std::shared_ptr<VKRenderSystem>& renderer, uint32_t vertexSize, uint32_t indexSize = sizeof(uint32_t),
		          vk::DeviceSize initialVertexBufferSize = 64 * 1024 * 1024, vk::DeviceSize initialIndexBufferSize = 16 * 1024 * 1024, vk::
		          DeviceSize vertexBufferBlockSize = 64 * 1024 * 1024, vk::DeviceSize indexBufferBlockSize = 16 * 1024 * 1024);

		void cleanup();

		bool addMesh(const std::shared_ptr<assets::StaticMeshAsset>& staticMesh);

		[[nodiscard]] bool hasMesh(const PuffinID staticMeshId) const
		{
			return mInternalMeshData.count(staticMeshId) == 1;
		}

		// CURRENTLY BROKEN, DO NOT USE!!!!!
		bool removeMeshes(const std::set<PuffinID>& staticMeshesToRemove);

		uint32_t meshVertexOffset(const PuffinID meshId, uint8_t subMeshIdx = 0)
		{
			return mInternalMeshData[meshId].subMeshData[subMeshIdx].vertexOffset;
		}

		uint32_t meshIndexOffset(const PuffinID meshId, uint8_t subMeshIdx = 0)
		{
			return mInternalMeshData[meshId].subMeshData[subMeshIdx].indexOffset;
		}

		uint32_t meshVertexCount(const PuffinID meshId, uint8_t subMeshIdx = 0)
		{
			return mInternalMeshData[meshId].subMeshData[subMeshIdx].vertexCount;
		}

		uint32_t meshIndexCount(const PuffinID meshId, uint8_t subMeshIdx = 0)
		{
			return mInternalMeshData[meshId].subMeshData[subMeshIdx].indexCount;
		}

		AllocatedBuffer& vertexBuffer() { return mVertexBuffer; }
		vk::DeviceAddress vertexBufferAddress() { return mVertexBufferAddress; }
		AllocatedBuffer& indexBuffer() { return mIndexBuffer; }

	private:

		std::shared_ptr<VKRenderSystem> mRenderer = nullptr;

		struct InternalSubMeshData
		{
			uint32_t vertexOffset, vertexCount;
			uint32_t indexOffset, indexCount;
		};

		struct InternalMeshData
		{
			std::vector<InternalSubMeshData> subMeshData;

			bool isActive;
		};

		std::unordered_map<PuffinID, InternalMeshData> mInternalMeshData;

		AllocatedBuffer mVertexBuffer;
		vk::DeviceAddress mVertexBufferAddress = {};
		AllocatedBuffer mIndexBuffer;

		uint32_t mVertexSize = 0; // Number of bytes for each vertex
		uint32_t mIndexSize = 0; // Number of bytes for each index

		// Offset into buffers where vertices/indices have been added
		uint32_t mVertexOffset = 0, mIndexOffset = 0;

		// Number of vertices/indices in use
		uint32_t mActiveVertexCount = 0, mActiveIndexCount = 0;

		// Total number of vertices/indices allocated in buffers
		uint32_t mMaxVertexCount = 0, mMaxIndexCount = 0;

		vk::DeviceSize mVertexBufferSize = 0, mIndexBufferSize = 0;
		vk::DeviceSize mVertexBufferBlockSize = 0;
		vk::DeviceSize mIndexBufferBlockSize = 0;

		// How much of buffer is unused before it should be shrank
		double mShrinkUsageThreshold = 0.5;

		bool growVertexBuffer(uint32_t minVertexCount);
		bool growIndexBuffer(uint32_t minIndexCount);

		bool shrinkVertexBuffer(uint32_t minVertexCount);
		bool shrinkIndexBuffer(uint32_t minVertexCount);

		AllocatedBuffer createVertexBuffer(vk::DeviceSize bufferSize);
		AllocatedBuffer createIndexBuffer(vk::DeviceSize bufferSize);
	};
}
