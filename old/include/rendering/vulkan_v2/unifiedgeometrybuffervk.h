#pragma once

#include <unordered_map>
#include <vector>
#include <set>

#include "puffin/rendering/vulkan/typesvk.h"
#include "puffin/assets/staticmeshasset.h"
#include "puffin/types/vertex.h"

namespace puffin::rendering
{
	class RenderSubsystemVK;

	struct UnifiedGeometryBufferParams
	{
		uint64_t vertexPageSize = 64 * 1024 * 1024;
		uint64_t vertexInitialPageCount = 1;
		uint64_t indexPageSize = 64 * 1024 * 1024;
		uint64_t indexInitialPageCount = 1;
	};

	// Custom buffer to store vertex/index data for multiple meshes in a single large vertex/index buffer
	class UnifiedGeometryBufferVK
	{
	public:

		explicit UnifiedGeometryBufferVK(RenderSubsystemVK* renderSystem, UnifiedGeometryBufferParams params = {});

		~UnifiedGeometryBufferVK();

		void AddStaticMesh(const std::shared_ptr<assets::StaticMeshAsset>& staticMesh);

		[[nodiscard]] bool HasMesh(const UUID staticMeshID) const;

		uint32_t MeshVertexOffset(const UUID meshID, uint8_t subMeshIdx = 0);
		uint32_t MeshIndexOffset(const UUID meshID, uint8_t subMeshIdx = 0);
		uint32_t MeshVertexCount(const UUID meshID, uint8_t subMeshIdx = 0);
		uint32_t MeshIndexCount(const UUID meshID, uint8_t subMeshIdx = 0);

		vk::DeviceAddress GetVertexBufferAddress(VertexFormat format = VertexFormat::PNTV32) const;
		AllocatedBuffer& GetVertexBuffer(VertexFormat format = VertexFormat::PNTV32);
		AllocatedBuffer& GetIndexBuffer();

	private:

		RenderSubsystemVK* mRenderSystem = nullptr;

		struct InternalVertexBufferData
		{
			InternalVertexBufferData() = default;

			explicit InternalVertexBufferData(const VertexFormat& format) : vertexFormat(format) {}

			VertexFormat vertexFormat;
			AllocatedBuffer allocBuffer;
			vk::DeviceAddress bufferAddress = {};
			uint64_t pageSize = 0;
			uint64_t pageCount = 0;
			uint64_t byteSize = 0; // Size of an individual vertex in bytes
			uint64_t byteSizeTotal = 0; // Total size of buffer in bytes
			uint64_t byteOffset = 0; // Offset into buffer in bytes currently in use
			uint64_t offset = 0; // 
		};

		struct InternalIndexBufferData
		{
			AllocatedBuffer allocBuffer;
			uint64_t pageCount = 0;
			uint64_t byteSize = 0;
			uint64_t byteSizeTotal = 0;
			uint64_t byteOffset = 0;
			uint64_t offset = 0;
		};

		struct InternalSubMeshData
		{
			uint32_t vertexOffset, vertexCount;
			uint32_t indexOffset, indexCount;
		};

		struct InternalMeshData
		{
			std::vector<InternalSubMeshData> subMeshData;

			bool active;
		};

		std::unordered_map<VertexFormat, InternalVertexBufferData> mVertexBufferData;
		InternalIndexBufferData mIndexBufferData;

		std::unordered_map<UUID, InternalMeshData> mInternalMeshData;

		vk::DeviceSize mVertexPageSize = 0;
		vk::DeviceSize mVertexInitialPageCount = 0;
		vk::DeviceSize mIndexPageSize = 0;
		vk::DeviceSize mIndexInitialPageCount = 0;

		void AddInternalVertexBuffer(VertexFormat format);

		void GrowVertexBuffer(InternalVertexBufferData& vertexBufferData, vk::DeviceSize minSize);
		void GrowIndexBuffer(InternalIndexBufferData& indexBufferData, vk::DeviceSize minSize);

		void ResizeVertexBuffer(InternalVertexBufferData& vertexBufferData, size_t newPageCount);
		void ResizeIndexBuffer(InternalIndexBufferData& indexBufferData, size_t newPageCount);

		AllocatedBuffer AllocateVertexBuffer(vk::DeviceSize bufferSize);
		AllocatedBuffer AllocateIndexBuffer(vk::DeviceSize bufferSize);
	};
}
