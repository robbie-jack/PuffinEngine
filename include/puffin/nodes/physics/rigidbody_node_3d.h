#pragma once

#include "puffin/nodes/transform_node_3d.h"

namespace puffin::physics
{
	class RigidbodyNode3D : public TransformNode3D
	{
	public:

		explicit RigidbodyNode3D(const std::shared_ptr<core::Engine>& engine, const PuffinID& id = gInvalidID);
		~RigidbodyNode3D() override = default;

		void begin_play() override;
		void update(const double delta_time) override;
		void physics_update(const double delta_time) override;
		void end_play() override;

	};
}