#pragma once

#include <unordered_set>
#include <memory>

#include "puffin/types/uuid.h"
#include "puffin/rendering/vulkan/types_vk.h"
#include "puffin/types/packed_vector.h"

namespace puffin::rendering
{
	class RenderSubystemVK;

	class MaterialRegistryVK
	{
	public:

		explicit MaterialRegistryVK(RenderSubystemVK* render_system);
		~MaterialRegistryVK();

		void register_material_instance(const PuffinID& id);

		void update();

		bool material_data_needs_uploaded() const { return mMaterialDataNeedsUploaded; }

		MaterialDataVK& get_material_data(const PuffinID& id) { return m_mat_data[id]; }
		PackedVector<PuffinID, MaterialDataVK>& get_material_data() { return m_mat_data; }

		MaterialVK& get_material(const PuffinID& id) { return m_mats[id]; }

		GPUMaterialInstanceData& get_cached_material_data(const PuffinID& id) { return m_cached_material_data[id]; }

	private:

		std::shared_ptr<RenderSubystemVK> m_render_system = nullptr;

		std::unordered_set<PuffinID> m_materials_to_load; // Materials that need to be loaded
		std::unordered_set<PuffinID> m_materials_instances_to_load; // Materials Instances that need to be loaded

		PackedVector<PuffinID, MaterialDataVK> m_mat_data;
		std::unordered_map<PuffinID, MaterialVK> m_mats;

		PackedVector<PuffinID, GPUMaterialInstanceData> m_cached_material_data; // Cached data for each unique material/instance

		bool mMaterialDataNeedsUploaded = false;

		bool load_material_instance(PuffinID matID, MaterialDataVK& matData);
		void init_material_pipeline(PuffinID matID);
		
	};
}
