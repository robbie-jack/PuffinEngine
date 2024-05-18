#include "puffin/rendering/vulkan/unified_geometry_buffer_vk.h"

#include "puffin/rendering/vulkan/helpers_vk.h"
#include "puffin/rendering/vulkan/render_system_vk.h"

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
	UnifiedGeometryBuffer::UnifiedGeometryBuffer(const std::shared_ptr<RenderSystemVK>& render_system,
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
			m_render_system->allocator().destroyBuffer(vertex_buffer_data.alloc_buffer.buffer, vertex_buffer_data.alloc_buffer.allocation);
		}

		m_vertex_buffer_data.clear();

		m_render_system->allocator().destroyBuffer(m_index_buffer_data.alloc_buffer.buffer, m_index_buffer_data.alloc_buffer.allocation);

		m_render_system = nullptr;

		m_index_buffer_data = {};
	}

	void UnifiedGeometryBuffer::add_static_mesh(const std::shared_ptr<assets::StaticMeshAsset>& static_mesh)
	{
		if (static_mesh && static_mesh->load())
		{
			if (m_internal_mesh_data.find(static_mesh->id()) == m_internal_mesh_data.end())
			{
				m_internal_mesh_data.emplace(static_mesh->id(), InternalMeshData());
			}

			if (m_vertex_buffer_data.find(static_mesh->vertex_format()) == m_vertex_buffer_data.end())
			{
				add_internal_vertex_buffer(static_mesh->vertex_format());
			}

			InternalVertexBufferData& internal_vertex_buffer_data = m_vertex_buffer_data[static_mesh->vertex_format()];

			auto& internal_mesh_data = m_internal_mesh_data.at(static_mesh->id());
			internal_mesh_data.active = true;

			const uint64_t new_vertex_byte_offset = internal_vertex_buffer_data.byte_offset + static_mesh->vertex_byte_size_total();
			if (new_vertex_byte_offset >= internal_vertex_buffer_data.byte_size_total)
			{
				grow_vertex_buffer(internal_vertex_buffer_data, new_vertex_byte_offset);
			}

			const uint64_t new_index_byte_offset = m_index_buffer_data.byte_offset + static_mesh->index_byte_size_total();
			if (new_index_byte_offset >= m_index_buffer_data.byte_size_total)
			{
				grow_index_buffer(m_index_buffer_data, new_index_byte_offset);
			}

			// Copy vertex data
			util::copy_cpu_data_into_gpu_buffer(m_render_system, internal_vertex_buffer_data.alloc_buffer, static_mesh->vertex_byte_size_total(),
				static_mesh->vertices().data(), 0, internal_vertex_buffer_data.byte_offset);

			// Copy index data
			util::copy_cpu_data_into_gpu_buffer(m_render_system, m_index_buffer_data.alloc_buffer, static_mesh->index_byte_size_total(),
				static_mesh->indices().data(), 0, m_index_buffer_data.byte_offset);

			for (const auto& sub_mesh_info : static_mesh->sub_mesh_info())
			{
				InternalSubMeshData sub_mesh_data = {};
				sub_mesh_data.vertex_offset = internal_vertex_buffer_data.offset + sub_mesh_info.vertex_offset;
				sub_mesh_data.index_offset = m_index_buffer_data.offset + sub_mesh_info.index_offset;
				sub_mesh_data.vertex_count = sub_mesh_info.vertex_count;
				sub_mesh_data.index_count = sub_mesh_info.index_count;

				internal_mesh_data.sub_mesh_data.push_back(sub_mesh_data);
			}

			internal_vertex_buffer_data.byte_offset = new_vertex_byte_offset;
			internal_vertex_buffer_data.offset += static_mesh->vertex_count_total();

			m_index_buffer_data.byte_offset = new_index_byte_offset;
			m_index_buffer_data.offset += static_mesh->index_count_total();
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
			util::copy_data_between_buffers(m_render_system, vertex_buffer_data.alloc_buffer.buffer,
				new_buffer.buffer, vertex_buffer_data.byte_offset);

			m_render_system->allocator().destroyBuffer(vertex_buffer_data.alloc_buffer.buffer, vertex_buffer_data.alloc_buffer.allocation);
		}

		vertex_buffer_data.alloc_buffer = new_buffer;
		vertex_buffer_data.buffer_address = m_render_system->device().getBufferAddress(vk::BufferDeviceAddressInfo{ vertex_buffer_data.alloc_buffer.buffer });
		vertex_buffer_data.page_count = new_page_count;
		vertex_buffer_data.byte_size_total = new_buffer_size;
	}

	void UnifiedGeometryBuffer::resize_index_buffer(InternalIndexBufferData& index_buffer_data, size_t new_page_count)
	{
		const uint64_t new_buffer_size = m_index_page_size * new_page_count;

		AllocatedBuffer new_buffer = allocate_index_buffer(new_buffer_size);

		if (index_buffer_data.byte_size_total > 0 && index_buffer_data.byte_offset > 0)
		{
			util::copy_data_between_buffers(m_render_system, index_buffer_data.alloc_buffer.buffer,
				new_buffer.buffer, index_buffer_data.byte_offset);

			m_render_system->allocator().destroyBuffer(index_buffer_data.alloc_buffer.buffer, index_buffer_data.alloc_buffer.allocation);
		}

		index_buffer_data.alloc_buffer = new_buffer;
		index_buffer_data.page_count = new_page_count;
		index_buffer_data.byte_size_total = new_buffer_size;
	}

	AllocatedBuffer UnifiedGeometryBuffer::allocate_vertex_buffer(vk::DeviceSize buffer_size)
	{
		return util::create_buffer(m_render_system->allocator(), buffer_size,
			{ vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddress },
			vma::MemoryUsage::eAutoPreferDevice,
			{ vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead |
			vma::AllocationCreateFlagBits::eMapped });
	}

	AllocatedBuffer UnifiedGeometryBuffer::allocate_index_buffer(vk::DeviceSize buffer_size)
	{
		return util::create_buffer(m_render_system->allocator(), buffer_size,
			{ vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc },
			vma::MemoryUsage::eAutoPreferDevice, { vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eHostAccessAllowTransferInstead |
			vma::AllocationCreateFlagBits::eMapped });
	}
}
