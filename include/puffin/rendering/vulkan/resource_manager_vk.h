#pragma once

#include <memory>

#include "puffin/rendering/vulkan/types_vk.h"
#include "puffin/types/packed_vector.h"
#include "puffin/types/uuid.h"

namespace puffin
{
	namespace assets
	{
		class StaticMeshAsset;
	}
}

namespace puffin::rendering
{
	class RenderSystemVK;
	class UnifiedGeometryBuffer;

	class ResourceManagerVK
	{
	public:

		explicit ResourceManagerVK(const std::shared_ptr<RenderSystemVK>& render_system);
		~ResourceManagerVK();

		void add_static_mesh(const std::shared_ptr<assets::StaticMeshAsset>& static_mesh);

		void add_images(PuffinID id, const ImageDesc& image_desc, uint8_t image_count);

		void destroy_images(PuffinID id);

		void update_image(PuffinID id, const ImageDesc& image_desc, uint8_t image_idx);
		void update_images(PuffinID id, const ImageDesc& image_desc);

		AllocatedImage& get_image(PuffinID id, uint8_t idx = 0);
		bool image_exists(PuffinID id) const;
		bool image_exists(PuffinID id, uint8_t idx) const;
		size_t image_count(PuffinID id) const;

		UnifiedGeometryBuffer* geometry_buffer() const { return m_unified_geometry_buffer; }

	private:

		std::shared_ptr<RenderSystemVK> m_render_system = nullptr;
		UnifiedGeometryBuffer* m_unified_geometry_buffer = nullptr;

		PackedVector<PuffinID, std::vector<AllocatedImage>> m_images;

		void create_image_internal(PuffinID id, const ImageDesc& image_desc, uint8_t idx = 0);
		void destroy_image_internal(PuffinID id, uint8_t idx = 0);
		void update_image_internal(PuffinID id, const ImageDesc& image_desc, uint8_t idx = 0);

	};
}
