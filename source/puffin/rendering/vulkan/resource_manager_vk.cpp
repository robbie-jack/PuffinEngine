#include "puffin/rendering/vulkan/resource_manager_vk.h"

#include "puffin/rendering/render_globals.h"
#include "puffin/rendering/vulkan/unified_geometry_buffer_vk.h"
#include "puffin/rendering/vulkan/helpers_vk.h"
#include "puffin/rendering/vulkan/render_system_vk.h"

namespace puffin::rendering
{
	ResourceManagerVK::ResourceManagerVK(RenderSystemVK* render_system)
		: m_render_system(render_system)
	{
		m_unified_geometry_buffer = new UnifiedGeometryBuffer(m_render_system);
	}

	ResourceManagerVK::~ResourceManagerVK()
	{
		for (auto images : m_images)
		{
			for (auto alloc_image : images)
			{
				m_render_system->device().destroyImageView(alloc_image.image_view);
				m_render_system->allocator().destroyImage(alloc_image.image, alloc_image.allocation);
			}
		}

		m_images.clear();

		m_render_system = nullptr;
	}

	void ResourceManagerVK::add_static_mesh(const std::shared_ptr<assets::StaticMeshAsset>& static_mesh)
	{
		m_unified_geometry_buffer->add_static_mesh(static_mesh);
	}

	void ResourceManagerVK::add_images(PuffinID id, const ImageDesc& image_desc, uint8_t image_count)
	{
		m_images.emplace(id, std::vector<AllocatedImage>());
		m_images.at(id).resize(image_count);

		for (int i = 0; i < image_count; ++i)
		{
			create_image_internal(id, image_desc, i);
		}
	}

	void ResourceManagerVK::destroy_images(PuffinID id)
	{
		if (m_images.contains(id))
		{
			for (int i = 0; i < m_images.at(id).size(); ++i)
			{
				destroy_image_internal(id, i);
			}

			m_images.clear();
		}
	}

	void ResourceManagerVK::update_image(PuffinID id, const ImageDesc& image_desc, uint8_t image_idx)
	{
		if (m_images.contains(id))
		{
			update_image_internal(id, image_desc, image_idx);
		}
	}

	void ResourceManagerVK::update_images(PuffinID id, const ImageDesc& image_desc)
	{
		if (m_images.contains(id))
		{
			for (int i = 0; i < m_images.at(id).size(); ++i)
			{
				update_image_internal(id, image_desc, i);
			}
		}
	}

	AllocatedImage& ResourceManagerVK::get_image(PuffinID id, uint8_t idx)
	{
		if (m_images.at(id).size() == 1)
		{
			return m_images.at(id)[0];
		}

		return m_images.at(id)[idx];
	}

	bool ResourceManagerVK::image_exists(PuffinID id) const
	{
		return m_images.contains(id);
	}

	bool ResourceManagerVK::image_exists(PuffinID id, uint8_t idx) const
	{
		return m_images.at(id).size() > idx;
	}

	size_t ResourceManagerVK::image_count(PuffinID id) const
	{
		return m_images.at(id).size();
	}

	void ResourceManagerVK::create_image_internal(PuffinID id, const ImageDesc& image_desc, uint8_t idx)
	{
		vk::Extent3D extent = { image_desc.width, image_desc.height, image_desc.depth };

		if (image_desc.image_type == ImageType::Color)
		{
			m_images.at(id)[idx] = util::create_color_image(m_render_system, extent, image_desc.format);
		}
		else
		{
			m_images.at(id)[idx] = util::create_depth_image(m_render_system, extent, image_desc.format);
		}
	}

	void ResourceManagerVK::destroy_image_internal(PuffinID id, uint8_t idx)
	{
		auto& alloc_image = m_images.at(id)[idx];

		m_render_system->device().destroyImageView(alloc_image.image_view);
		m_render_system->allocator().destroyImage(alloc_image.image, alloc_image.allocation);
	}

	void ResourceManagerVK::update_image_internal(PuffinID id, const ImageDesc& image_desc, uint8_t idx)
	{
		destroy_image_internal(id, idx);
		create_image_internal(id, image_desc, idx);
	}
}
