#pragma once

#include "puffin/nodes/transformnode3d.h"

namespace puffin::physics
{
	class RigidbodyNode3D : public TransformNode3D
	{
	public:

		explicit RigidbodyNode3D(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
		~RigidbodyNode3D() override = default;

		void begin_play() override;
		void update(const double delta_time) override;
		void update_fixed(const double delta_time) override;
		void end_play() override;

	};
}