#include "puffin/rendering/vulkan/resource_manager_vk.h"

#include "puffin/rendering/vulkan/unified_geometry_buffer_vk.h"
#include "puffin/rendering/vulkan/render_system_vk.h"

namespace puffin::rendering
{
	ResourceManagerVK::ResourceManagerVK(const std::shared_ptr<RenderSystemVK>& render_system) : m_render_system(render_system)
	{
		m_unified_geometry_buffer = new UnifiedGeometryBuffer(m_render_system);
	}

	void ResourceManagerVK::add_static_mesh(const std::shared_ptr<assets::StaticMeshAsset>& static_mesh)
	{
		m_unified_geometry_buffer->add_static_mesh(static_mesh);
	}
}
