#pragma once

#include "puffin/nodes/transform_node_3d.h"

namespace puffin::rendering
{
	class MeshNode : public TransformNode3D
	{
	public:

		explicit MeshNode(const std::shared_ptr<core::Engine>& engine, const PuffinID& id = gInvalidID);

		void begin_play() override;
		void update(const double delta_time) override;
		void update_fixed(const double delta_time) override;
		void end_play() override;

		PuffinID mesh_asset_id();
		void set_mesh_asset_id(PuffinID mesh_asset_id) const;

		PuffinID mat_asset_id();
		void set_mat_asset_id(PuffinID mat_asset_id) const;

		uint8_t sub_mesh_idx();
		void set_sub_mesh_idx(uint8_t sub_mesh_idx) const;

	private:



	};
}