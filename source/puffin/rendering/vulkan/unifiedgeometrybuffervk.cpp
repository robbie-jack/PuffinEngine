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
	UnifiedGeometryBuffer::UnifiedGeometryBuffer(RenderSubystemVK* render_system,
		uint64_t vertex_page_size, uint64_t vertex_initial_page_count, uint64_t index_page_size,
		uint64_t index_initial_page_count) :
		m_render_system(render_system), m_vertex_page_size(vertex_page_size), m_vertex_initial_page_count(vertex_initial_page_count),
		m_index_page_size(index_page_size), m_index_initial_page_count(index_initial_page_count)
	{
		add_internal_vertex_buffer(VertexFormat::PNTV32);

		resize_index_buffer(m_index_buffer_data, m_index_initial_page_count);
	}

	UnifiedGeometryBuffer::~UnifiedGeometryBuffer()
	{
		m_vertex_page_size = 0;
		m_index_page_size = 0;
		m_index_initial_page_count = 0;

		for (auto& [vertex_format, vertex_buffer_data] : m_vertex_buffer_data)
		{
			m_render_system->GetAllocator().destroyBuffer(vertex_buffer_data.alloc_buffer.buffer, vertex_buffer_data.alloc_buffer.allocation);
		}

		m_vertex_buffer_data.clear();

		m_render_system->GetAllocator().destroyBuffer(m_index_buffer_data.alloc_buffer.buffer, m_index_buffer_data.alloc_buffer.allocation);

		m_render_system = nullptr;

		m_index_buffer_data = {};
	}

	void UnifiedGeometryBuffer::add_static_mesh(const std::shared_ptr<assets::StaticMeshAsset>& static_mesh)
	{
		if (static_mesh && static_mesh->Load())
		{
			if (m_internal_mesh_data.find(static_mesh->GetID()) == m_internal_mesh_data.end())
			{
				m_internal_mesh_data.emplace(static_mesh->GetID(), InternalMeshData());
			}

			if (m_vertex_buffer_data.find(static_mesh->VertexFormat()) == m_vertex_buffer_data.end())
			{
				add_internal_vertex_buffer(static_mesh->VertexFormat());
			}

			InternalVertexBufferData& internal_vertex_buffer_data = m_vertex_buffer_data[static_mesh->VertexFormat()];

			auto& internal_mesh_data = m_internal_mesh_data.at(static_mesh->GetID());
			internal_mesh_data.active = true;

			const uint64_t new_vertex_byte_offset = internal_vertex_buffer_data.byte_offset + static_mesh->VertexByteSizeTotal();
			if (new_vertex_byte_offset >= internal_vertex_buffer_data.byte_size_total)
			{
				grow_vertex_buffer(internal_vertex_buffer_data, new_vertex_byte_offset);
			}

			const uint64_t new_index_byte_offset = m_index_buffer_data.byte_offset + static_mesh->IndexByteSizeTotal();
			if (new_index_byte_offset >= m_index_buffer_data.byte_size_total)
			{
				grow_index_buffer(m_index_buffer_data, new_index_byte_offset);
			}

			// Copy vertex data
			util::CopyCPUDataIntoGPUBuffer(m_render_system, TODO);

			// Copy index data
			util::CopyCPUDataIntoGPUBuffer(m_render_system, TODO);

			for (const auto& sub_mesh_info : static_mesh->SubMeshInfo())
			{
				InternalSubMeshData sub_mesh_data = {};
				sub_mesh_data.vertex_offset = internal_vertex_buffer_data.offset + sub_mesh_info.vertexOffset;
				sub_mesh_data.index_offset = m_index_buffer_data.offset + sub_mesh_info.indexOffset;
				sub_mesh_data.vertex_count = sub_mesh_info.vertexCount;
				sub_mesh_data.index_count = sub_mesh_info.indexCount;

				internal_mesh_data.sub_mesh_data.push_back(sub_mesh_data);
			}

			internal_vertex_buffer_data.byte_offset = new_vertex_byte_offset;
			internal_vertex_buffer_data.offset += static_mesh->VertexCountTotal();

			m_index_buffer_data.byte_offset = new_index_byte_offset;
			m_index_buffer_data.offset += static_mesh->IndexCountTotal();
		}
	}

	void UnifiedGeometryBuffer::add_internal_vertex_buffer(VertexFormat format)
	{
		m_vertex_buffer_data.emplace(format, InternalVertexBufferData(format));

		m_vertex_buffer_data[format].byte_size = parseVertexSizeFromFormat(format);

		resize_vertex_buffer(m_vertex_buffer_data[format], m_vertex_initial_page_count);
	}

	void UnifiedGeometryBuffer::grow_vertex_buffer(InternalVertexBufferData& vertex_buffer_data,
	                                               vk::DeviceSize min_size)
	{
		if (min_size > vertex_buffer_data.byte_size_total)
		{
			const int min_page_count = std::ceil(min_size / m_vertex_page_size);

			resize_vertex_buffer(vertex_buffer_data, min_page_count);
		}
	}

	void UnifiedGeometryBuffer::grow_index_buffer(InternalIndexBufferData& index_buffer_data,
	                                              vk::DeviceSize min_size)
	{
		if (min_size > index_buffer_data.byte_size_total)
		{
			const int min_page_count = std::ceil(min_size / m_index_page_size);

			resize_index_buffer(index_buffer_data, min_page_count);
		}
	}

	void UnifiedGeometryBuffer::resize_vertex_buffer(InternalVertexBufferData& vertex_buffer_data,
		size_t new_page_count)
	{
		const uint64_t new_buffer_size = m_vertex_page_size * new_page_count;

		AllocatedBuffer new_buffer = allocate_vertex_buffer(new_buffer_size);

		if (vertex_buffer_data.byte_size_total > 0 && vertex_buffer_data.byte_offset > 0)
		{
			util::CopyDataBetweenBuffersParams params;
			params.dataSize = vertex_buffer_data.byte_offset;
			params.srcBuffer = vertex_buffer_data.alloc_buffer.buffer;
			params.dstBuffer = new_buffer.buffer;
			util::CopyDataBetweenBuffers(m_render_system, params);

			m_render_system->GetAllocator().destroyBuffer(vertex_buffer_data.alloc_buffer.buffer, vertex_buffer_data.alloc_buffer.allocation);
		}

		vertex_buffer_data.alloc_buffer = new_buffer;
		vertex_buffer_data.buffer_address = m_render_system->GetDevice().getBufferAddress(vk::BufferDeviceAddressInfo{ vertex_buffer_data.alloc_buffer.buffer });
		vertex_buffer_data.page_count = new_page_count;
		vertex_buffer_data.byte_size_total = new_buffer_size;
	}

	void UnifiedGeometryBuffer::resize_index_buffer(InternalIndexBufferData& index_buffer_data, size_t new_page_count)
	{
		const uint64_t new_buffer_size = m_index_page_size * new_page_count;

		AllocatedBuffer new_buffer = allocate_index_buffer(new_buffer_size);

		if (index_buffer_data.byte_size_total > 0 && index_buffer_data.byte_offset > 0)
		{
			util::CopyDataBetweenBuffersParams params;
			params.dataSize = index_buffer_data.byte_offset;
			params.srcBuffer = index_buffer_data.alloc_buffer.buffer;
			params.dstBuffer = new_buffer.buffer;
			util::CopyDataBetweenBuffers(m_render_system, params);

			m_render_system->GetAllocator().destroyBuffer(index_buffer_data.alloc_buffer.buffer, index_buffer_data.alloc_buffer.allocation);
		}

		index_buffer_data.alloc_buffer = new_buffer;
		index_buffer_data.page_count = new_page_count;
		index_buffer_data.byte_size_total = new_buffer_size;
	}

	AllocatedBuffer UnifiedGeometryBuffer::allocate_vertex_buffer(vk::DeviceSize buffer_size)
	{
		util::CreateBufferParams params;
		params.allocSize = buffer_size;
		params.bufferUsage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | 
			vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddress;
		params.memoryUsage = vma::MemoryUsage::eAutoPreferDevice;
		params.allocFlags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | 
			vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead |
			vma::AllocationCreateFlagBits::eMapped;
		return util::CreateBuffer(m_render_system->GetAllocator(), params);
	}

	AllocatedBuffer UnifiedGeometryBuffer::allocate_index_buffer(vk::DeviceSize buffer_size)
	{
		util::CreateBufferParams params;
		params.allocSize = buffer_size;
		params.bufferUsage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst |
			vk::BufferUsageFlagBits::eTransferSrc;
		params.memoryUsage = vma::MemoryUsage::eAutoPreferDevice;
		params.allocFlags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite |
			vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead |
			vma::AllocationCreateFlagBits::eMapped;
		return util::CreateBuffer(m_render_system->GetAllocator(), params);
	}
}
