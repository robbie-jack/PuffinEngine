#pragma once

#include "puffin/types/uuid.h"

#include <string>
#include <unordered_set>

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
		~ResourceManagerVK() = default;

		void add_static_mesh(const std::shared_ptr<assets::StaticMeshAsset>& static_mesh);

		UnifiedGeometryBuffer* geometry_buffer() const { return m_unified_geometry_buffer; }

	private:

		std::shared_ptr<RenderSystemVK> m_render_system = nullptr;
		UnifiedGeometryBuffer* m_unified_geometry_buffer = nullptr;

	};
}
