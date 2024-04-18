#pragma once

#include "scene/node.h"

namespace puffin
{
	struct TransformComponent3D;
}

namespace puffin::scene
{
	class TransformNode3D : public Node
	{
	public:

		explicit  TransformNode3D(const PuffinID& id = gInvalidID);
		~TransformNode3D() override = default;

		void begin_play() override;
		void update(double delta_time) override;
		void physics_update(double delta_time) override;
		void end_play() override;

		TransformComponent3D& get_transform();

	protected:



	};
}