#include "puffin/rendering/vulkan/resourcemanagervk.h"

#include "puffin/rendering/renderglobals.h"
#include "puffin/rendering/vulkan/unifiedgeometrybuffervk.h"
#include "puffin/rendering/vulkan/helpersvk.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"

namespace puffin::rendering
{
	ResourceManagerVK::ResourceManagerVK(RenderSubystemVK* render_system)
		: m_render_system(render_system)
	{
		m_unified_geometry_buffer = new UnifiedGeometryBuffer(m_render_system);
	}

	ResourceManagerVK::~ResourceManagerVK()
	{
		if (m_images.size() > 0)
		{
			for (auto images : m_images)
			{
				for (auto alloc_image : images)
				{
					m_render_system->GetDevice().destroyImageView(alloc_image.image_view);
					m_render_system->GetAllocator().destroyImage(alloc_image.image, alloc_image.allocation);
				}
			}

			m_images.clear();
		}

		m_render_system = nullptr;
	}

	void ResourceManagerVK::add_static_mesh(const std::shared_ptr<assets::StaticMeshAsset>& static_mesh)
	{
		m_unified_geometry_buffer->add_static_mesh(static_mesh);
	}

	ResourceID ResourceManagerVK::add_images(const ImageDesc& image_desc, uint8_t image_count)
	{
		ResourceID id = GenerateId();

		m_images.emplace(id, std::vector<AllocatedImage>());
		m_images.at(id).resize(image_count);

		for (int i = 0; i < image_count; ++i)
		{
			create_image_internal(id, image_desc, i);
		}

		return id;
	}

	void ResourceManagerVK::destroy_images(ResourceID id)
	{
		if (m_images.contains(id))
		{
			for (int i = 0; i < m_images.at(id).size(); ++i)
			{
				destroy_image_internal(id, i);
			}

			m_images.erase(id);
		}
	}

	void ResourceManagerVK::update_image(ResourceID id, const ImageDesc& image_desc, uint8_t image_idx)
	{
		assert(m_images.contains(id) && "ResourceManagerVK::update_images - Atempting to get image with invalid resource id");

		if (m_images.contains(id))
		{
			update_image_internal(id, image_desc, image_idx);
		}
	}

	void ResourceManagerVK::update_images(ResourceID id, const ImageDesc& image_desc)
	{
		assert(m_images.contains(id) && "ResourceManagerVK::update_images - Atempting to get image with invalid resource id");

		if (m_images.contains(id))
		{
			for (int i = 0; i < m_images.at(id).size(); ++i)
			{
				update_image_internal(id, image_desc, i);
			}
		}
	}

	AllocatedImage& ResourceManagerVK::get_image(ResourceID id, uint8_t idx)
	{
		assert(m_images.contains(id) && "ResourceManagerVK::get_image - Atempting to get image with invalid resource id");

		if (m_images.at(id).size() == 1)
		{
			return m_images.at(id)[0];
		}

		return m_images.at(id)[idx];
	}

	bool ResourceManagerVK::image_exists(ResourceID id) const
	{
		return m_images.contains(id);
	}

	bool ResourceManagerVK::image_exists(ResourceID id, uint8_t idx) const
	{
		return m_images.at(id).size() > idx;
	}

	size_t ResourceManagerVK::image_count(ResourceID id) const
	{
		assert(m_images.contains(id) && "ResourceManagerVK::image_count - Atempting to get image with invalid resource id");

		return m_images.at(id).size();
	}

	void ResourceManagerVK::create_image_internal(ResourceID id, const ImageDesc& image_desc, uint8_t idx)
	{
		const vk::Extent3D extent = { image_desc.width, image_desc.height, image_desc.depth };

		util::CreateFormattedImageParams params;
		params.extent = extent;
		params.format = image_desc.format;

		if (image_desc.image_type == ImageType::Color)
		{
			m_images.at(id)[idx] = util::CreateColorImage(m_render_system, params);
		}
		else
		{
			m_images.at(id)[idx] = util::CreateDepthImage(m_render_system, params);
		}
	}

	void ResourceManagerVK::destroy_image_internal(ResourceID id, uint8_t idx)
	{
		auto& alloc_image = m_images.at(id)[idx];

		m_render_system->GetDevice().destroyImageView(alloc_image.image_view);
		m_render_system->GetAllocator().destroyImage(alloc_image.image, alloc_image.allocation);
	}

	void ResourceManagerVK::update_image_internal(ResourceID id, const ImageDesc& image_desc, uint8_t idx)
	{
		destroy_image_internal(id, idx);
		create_image_internal(id, image_desc, idx);
	}
}
