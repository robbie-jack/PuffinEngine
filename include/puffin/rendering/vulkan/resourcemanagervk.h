#pragma once

#include <memory>

#include "puffin/rendering/vulkan/typesvk.h"
#include "puffin/types/packedvector.h"
#include "puffin/types/uuid.h"
#include "puffin/rendering/resourceid.h"

namespace puffin
{
	namespace assets
	{
		class StaticMeshAsset;
	}
}

namespace puffin::rendering
{
	class RenderSubystemVK;
	class UnifiedGeometryBuffer;

	class ResourceManagerVK
	{
	public:

		explicit ResourceManagerVK(RenderSubystemVK* render_system);
		~ResourceManagerVK();

		void add_static_mesh(const std::shared_ptr<assets::StaticMeshAsset>& static_mesh);

		ResourceID add_images(const ImageDesc& image_desc, uint8_t image_count);

		void destroy_images(ResourceID id);

		void update_image(ResourceID id, const ImageDesc& image_desc, uint8_t image_idx);
		void update_images(ResourceID id, const ImageDesc& image_desc);

		AllocatedImage& get_image(ResourceID id, uint8_t idx = 0);
		bool image_exists(ResourceID id) const;
		bool image_exists(ResourceID id, uint8_t idx) const;
		size_t image_count(ResourceID id) const;

		UnifiedGeometryBuffer* geometry_buffer() const { return m_unified_geometry_buffer; }

	private:

		RenderSubystemVK* m_render_system = nullptr;
		UnifiedGeometryBuffer* m_unified_geometry_buffer = nullptr;

		PackedVector<ResourceID, std::vector<AllocatedImage>> m_images;

		void create_image_internal(ResourceID id, const ImageDesc& image_desc, uint8_t idx = 0);
		void destroy_image_internal(ResourceID id, uint8_t idx = 0);
		void update_image_internal(ResourceID id, const ImageDesc& image_desc, uint8_t idx = 0);

	};
}
