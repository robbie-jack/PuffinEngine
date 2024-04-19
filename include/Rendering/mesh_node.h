#pragma once

#include "scene/transform_node_3d.h"

namespace puffin::rendering
{
	class MeshNode : public scene::TransformNode3D
	{
	public:

		explicit MeshNode(const std::shared_ptr<core::Engine>& engine, const PuffinID& id = gInvalidID);

		void begin_play() override;
		void update(const double delta_time) override;
		void physics_update(const double delta_time) override;
		void end_play() override;

		PuffinID& mesh_asset_id();
		PuffinID& mat_asset_id();
		uint8_t sub_mesh_idx();

	private:



	};
}