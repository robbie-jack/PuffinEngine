#include "puffin/rendering/vulkan/unifiedgeometrybuffervk.h"

#include "puffin/rendering/vulkan/helpersvk.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"

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

namespace puffin::rendering
{
	UnifiedGeometryBufferVK::UnifiedGeometryBufferVK(RenderSubsystemVK* renderSystem, UnifiedGeometryBufferParams params) :
		mRenderSystem(renderSystem), mVertexPageSize(params.vertexPageSize), mVertexInitialPageCount(params.vertexInitialPageCount),
		mIndexPageSize(params.indexPageSize), mIndexInitialPageCount(params.indexInitialPageCount)
	{
		AddInternalVertexBuffer(VertexFormat::PNTV32);

		ResizeIndexBuffer(mIndexBufferData, mIndexInitialPageCount);
	}

	UnifiedGeometryBufferVK::~UnifiedGeometryBufferVK()
	{
		mVertexPageSize = 0;
		mIndexPageSize = 0;
		mIndexInitialPageCount = 0;

		for (auto& [vertex_format, vertex_buffer_data] : mVertexBufferData)
		{
			mRenderSystem->GetAllocator().destroyBuffer(vertex_buffer_data.allocBuffer.buffer, vertex_buffer_data.allocBuffer.allocation);
		}

		mVertexBufferData.clear();

		mRenderSystem->GetAllocator().destroyBuffer(mIndexBufferData.allocBuffer.buffer, mIndexBufferData.allocBuffer.allocation);

		mRenderSystem = nullptr;

		mIndexBufferData = {};
	}

	void UnifiedGeometryBufferVK::AddStaticMesh(const std::shared_ptr<assets::StaticMeshAsset>& staticMesh)
	{
		if (staticMesh && staticMesh->Load())
		{
			if (mInternalMeshData.find(staticMesh->GetID()) == mInternalMeshData.end())
			{
				mInternalMeshData.emplace(staticMesh->GetID(), InternalMeshData());
			}

			if (mVertexBufferData.find(staticMesh->VertexFormat()) == mVertexBufferData.end())
			{
				AddInternalVertexBuffer(staticMesh->VertexFormat());
			}

			InternalVertexBufferData& internalVertexBufferData = mVertexBufferData[staticMesh->VertexFormat()];

			auto& internalMeshData = mInternalMeshData.at(staticMesh->GetID());
			internalMeshData.active = true;

			const uint64_t newVertexByteOffset = internalVertexBufferData.byteOffset + staticMesh->VertexByteSizeTotal();
			if (newVertexByteOffset >= internalVertexBufferData.byteSizeTotal)
			{
				GrowVertexBuffer(internalVertexBufferData, newVertexByteOffset);
			}

			const uint64_t newIndexByteOffset = mIndexBufferData.byteOffset + staticMesh->IndexByteSizeTotal();
			if (newIndexByteOffset >= mIndexBufferData.byteSizeTotal)
			{
				GrowIndexBuffer(mIndexBufferData, newIndexByteOffset);
			}

			// Copy vertex data
			util::CopyCPUDataIntoGPUBufferParams params;
			params.dstBuffer = internalVertexBufferData.allocBuffer;
			params.dataSize = staticMesh->VertexByteSizeTotal();
			params.srcData = staticMesh->Vertices().data();
			params.dstOffset = internalVertexBufferData.byteOffset;

			util::CopyCPUDataIntoGPUBuffer(mRenderSystem, params);

			// Copy index data
			params.dstBuffer = mIndexBufferData.allocBuffer;
			params.dataSize = staticMesh->IndexByteSizeTotal();
			params.srcData = staticMesh->Indices().data();
			params.dstOffset = mIndexBufferData.byteOffset;

			util::CopyCPUDataIntoGPUBuffer(mRenderSystem, params);

			for (const auto& subMeshInfo : staticMesh->SubMeshInfo())
			{
				InternalSubMeshData subMeshData = {};
				subMeshData.vertexOffset = internalVertexBufferData.offset + subMeshInfo.vertexOffset;
				subMeshData.indexOffset = mIndexBufferData.offset + subMeshInfo.indexOffset;
				subMeshData.vertexCount = subMeshInfo.vertexCount;
				subMeshData.indexCount = subMeshInfo.indexCount;

				mInternalMeshData[staticMesh->GetID()].subMeshData.push_back(subMeshData);
			}

			internalVertexBufferData.byteOffset = newVertexByteOffset;
			internalVertexBufferData.offset += staticMesh->VertexCountTotal();

			mIndexBufferData.byteOffset = newIndexByteOffset;
			mIndexBufferData.offset += staticMesh->IndexCountTotal();
		}
	}

	bool UnifiedGeometryBufferVK::HasMesh(const UUID staticMeshID) const
	{
		return mInternalMeshData.count(staticMeshID) == 1;
	}

	uint32_t UnifiedGeometryBufferVK::MeshVertexOffset(const UUID meshID, uint8_t subMeshIdx)
	{
		return mInternalMeshData[meshID].subMeshData[subMeshIdx].vertexOffset;
	}

	uint32_t UnifiedGeometryBufferVK::MeshIndexOffset(const UUID meshID, uint8_t subMeshIdx)
	{
		return mInternalMeshData[meshID].subMeshData[subMeshIdx].indexOffset;
	}

	uint32_t UnifiedGeometryBufferVK::MeshVertexCount(const UUID meshID, uint8_t subMeshIdx)
	{
		return mInternalMeshData[meshID].subMeshData[subMeshIdx].vertexCount;
	}

	uint32_t UnifiedGeometryBufferVK::MeshIndexCount(const UUID meshID, uint8_t subMeshIdx)
	{
		return mInternalMeshData[meshID].subMeshData[subMeshIdx].indexCount;
	}

	vk::DeviceAddress UnifiedGeometryBufferVK::GetVertexBufferAddress(VertexFormat format) const
	{
		return mVertexBufferData.at(format).bufferAddress;
	}

	AllocatedBuffer& UnifiedGeometryBufferVK::GetVertexBuffer(VertexFormat format)
	{
		return mVertexBufferData.at(format).allocBuffer;
	}

	AllocatedBuffer& UnifiedGeometryBufferVK::GetIndexBuffer()
	{
		return mIndexBufferData.allocBuffer;
	}

	void UnifiedGeometryBufferVK::AddInternalVertexBuffer(VertexFormat format)
	{
		mVertexBufferData.emplace(format, InternalVertexBufferData(format));

		mVertexBufferData[format].byteSize = parseVertexSizeFromFormat(format);

		ResizeVertexBuffer(mVertexBufferData[format], mVertexInitialPageCount);
	}

	void UnifiedGeometryBufferVK::GrowVertexBuffer(InternalVertexBufferData& vertexBufferData,
	                                               vk::DeviceSize minSize)
	{
		if (minSize > vertexBufferData.byteSizeTotal)
		{
			const int minPageCount = std::ceil(minSize / mVertexPageSize);

			ResizeVertexBuffer(vertexBufferData, minPageCount);
		}
	}

	void UnifiedGeometryBufferVK::GrowIndexBuffer(InternalIndexBufferData& indexBufferData,
	                                              vk::DeviceSize minSize)
	{
		if (minSize > indexBufferData.byteSizeTotal)
		{
			const int minPageCount = std::ceil(minSize / mIndexPageSize);

			ResizeIndexBuffer(indexBufferData, minPageCount);
		}
	}

	void UnifiedGeometryBufferVK::ResizeVertexBuffer(InternalVertexBufferData& vertexBufferData,
		size_t newPageCount)
	{
		const uint64_t newBufferSize = mVertexPageSize * newPageCount;

		const AllocatedBuffer newBuffer = AllocateVertexBuffer(newBufferSize);

		if (vertexBufferData.byteSizeTotal > 0 && vertexBufferData.byteOffset > 0)
		{
			util::CopyDataBetweenBuffersParams params;
			params.dataSize = vertexBufferData.byteOffset;
			params.srcBuffer = vertexBufferData.allocBuffer.buffer;
			params.dstBuffer = newBuffer.buffer;
			util::CopyDataBetweenBuffers(mRenderSystem, params);

			mRenderSystem->GetAllocator().destroyBuffer(vertexBufferData.allocBuffer.buffer, vertexBufferData.allocBuffer.allocation);
		}

		vertexBufferData.allocBuffer = newBuffer;
		vertexBufferData.bufferAddress = mRenderSystem->GetDevice().getBufferAddress(vk::BufferDeviceAddressInfo{ vertexBufferData.allocBuffer.buffer });
		vertexBufferData.pageCount = newPageCount;
		vertexBufferData.byteSizeTotal = newBufferSize;
	}

	void UnifiedGeometryBufferVK::ResizeIndexBuffer(InternalIndexBufferData& indexBufferData, size_t newPageCount)
	{
		const uint64_t newBufferSize = mIndexPageSize * newPageCount;

		const AllocatedBuffer newBuffer = AllocateIndexBuffer(newBufferSize);

		if (indexBufferData.byteSizeTotal > 0 && indexBufferData.byteOffset > 0)
		{
			util::CopyDataBetweenBuffersParams params;
			params.dataSize = indexBufferData.byteOffset;
			params.srcBuffer = indexBufferData.allocBuffer.buffer;
			params.dstBuffer = newBuffer.buffer;
			util::CopyDataBetweenBuffers(mRenderSystem, params);

			mRenderSystem->GetAllocator().destroyBuffer(indexBufferData.allocBuffer.buffer, indexBufferData.allocBuffer.allocation);
		}

		indexBufferData.allocBuffer = newBuffer;
		indexBufferData.pageCount = newPageCount;
		indexBufferData.byteSizeTotal = newBufferSize;
	}

	AllocatedBuffer UnifiedGeometryBufferVK::AllocateVertexBuffer(vk::DeviceSize bufferSize)
	{
		util::CreateBufferParams params;
		params.allocSize = bufferSize;
		params.bufferUsage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | 
			vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddress;
		params.memoryUsage = vma::MemoryUsage::eAutoPreferDevice;
		params.allocFlags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | 
			vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead |
			vma::AllocationCreateFlagBits::eMapped;
		return util::CreateBuffer(mRenderSystem->GetAllocator(), params);
	}

	AllocatedBuffer UnifiedGeometryBufferVK::AllocateIndexBuffer(vk::DeviceSize bufferSize)
	{
		util::CreateBufferParams params;
		params.allocSize = bufferSize;
		params.bufferUsage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst |
			vk::BufferUsageFlagBits::eTransferSrc;
		params.memoryUsage = vma::MemoryUsage::eAutoPreferDevice;
		params.allocFlags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite |
			vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead |
			vma::AllocationCreateFlagBits::eMapped;
		return util::CreateBuffer(mRenderSystem->GetAllocator(), params);
	}
}
