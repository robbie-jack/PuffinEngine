#pragma once

#include <unordered_map>
#include <vector>
#include <set>

#include "puffin/rendering/vulkan/typesvk.h"
#include "puffin/assets/staticmeshasset.h"
#include "puffin/types/vertex.h"

namespace puffin::rendering
{
	class RenderSubystemVK;

	// Custom buffer to store vertex/index data for multiple meshes in a single large vertex/index buffer
	class UnifiedGeometryBuffer
	{
	public:

		explicit UnifiedGeometryBuffer(RenderSubystemVK* render_system, uint64_t vertex_page_size = 64 * 1000 * 1000,
		                               uint64_t vertex_initial_page_count = 1, uint64_t index_page_size = 64 * 1000 * 1000, uint64_t index_initial_page_count = 1);

		~UnifiedGeometryBuffer();

		void add_static_mesh(const std::shared_ptr<assets::StaticMeshAsset>& static_mesh);

		[[nodiscard]] bool hasMesh(const PuffinID staticMeshId) const
		{
			return m_internal_mesh_data.count(staticMeshId) == 1;
		}

		uint32_t mesh_vertex_offset(const PuffinID meshId, uint8_t subMeshIdx = 0)
		{
			return m_internal_mesh_data[meshId].sub_mesh_data[subMeshIdx].vertex_offset;
		}

		uint32_t mesh_index_offset(const PuffinID meshId, uint8_t subMeshIdx = 0)
		{
			return m_internal_mesh_data[meshId].sub_mesh_data[subMeshIdx].index_offset;
		}

		uint32_t mesh_vertex_count(const PuffinID meshId, uint8_t subMeshIdx = 0)
		{
			return m_internal_mesh_data[meshId].sub_mesh_data[subMeshIdx].vertex_count;
		}

		uint32_t mesh_index_count(const PuffinID meshId, uint8_t subMeshIdx = 0)
		{
			return m_internal_mesh_data[meshId].sub_mesh_data[subMeshIdx].index_count;
		}

		vk::DeviceAddress vertex_buffer_address(VertexFormat format = VertexFormat::PNTV32) { return m_vertex_buffer_data.at(format).buffer_address; }
		AllocatedBuffer& vertex_buffer(VertexFormat format = VertexFormat::PNTV32) { return m_vertex_buffer_data.at(format).alloc_buffer; }
		AllocatedBuffer& index_buffer() { return m_index_buffer_data.alloc_buffer; }

	private:

		RenderSubystemVK* m_render_system = nullptr;

		struct InternalVertexBufferData
		{
			InternalVertexBufferData() = default;

			explicit InternalVertexBufferData(const VertexFormat& format) : vertex_format(format) {}

			VertexFormat vertex_format;
			AllocatedBuffer alloc_buffer;
			vk::DeviceAddress buffer_address = {};
			uint64_t page_size = 0;
			uint64_t page_count = 0;
			uint64_t byte_size = 0; // Size of an individual vertex in bytes
			uint64_t byte_size_total = 0; // Total size of buffer in bytes
			uint64_t byte_offset = 0; // Offset into buffer in bytes currently in use
			uint64_t offset = 0; // 
		};

		struct InternalIndexBufferData
		{
			AllocatedBuffer alloc_buffer;
			uint64_t page_count = 0;
			uint64_t byte_size = 0;
			uint64_t byte_size_total = 0;
			uint64_t byte_offset = 0;
			uint64_t offset = 0;
		};

		struct InternalSubMeshData
		{
			uint32_t vertex_offset, vertex_count;
			uint32_t index_offset, index_count;
		};

		struct InternalMeshData
		{
			std::vector<InternalSubMeshData> sub_mesh_data;

			bool active;
		};

		std::unordered_map<VertexFormat, InternalVertexBufferData> m_vertex_buffer_data;
		InternalIndexBufferData m_index_buffer_data;

		std::unordered_map<PuffinID, InternalMeshData> m_internal_mesh_data;

		vk::DeviceSize m_vertex_page_size = 0;
		vk::DeviceSize m_vertex_initial_page_count = 0;
		vk::DeviceSize m_index_page_size = 0;
		vk::DeviceSize m_index_initial_page_count = 0;

		void add_internal_vertex_buffer(VertexFormat format);

		void grow_vertex_buffer(InternalVertexBufferData& vertex_buffer_data, vk::DeviceSize min_size);
		void grow_index_buffer(InternalIndexBufferData& index_buffer_data, vk::DeviceSize min_size);

		void resize_vertex_buffer(InternalVertexBufferData& vertex_buffer_data, size_t new_page_count);
		void resize_index_buffer(InternalIndexBufferData& index_buffer_data, size_t new_page_count);

		AllocatedBuffer allocate_vertex_buffer(vk::DeviceSize buffer_size);
		AllocatedBuffer allocate_index_buffer(vk::DeviceSize buffer_size);
	};
}
