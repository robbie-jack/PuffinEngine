#pragma once

#include "scene/node.h"

namespace puffin
{
	struct TransformComponent2D;
}

namespace puffin::scene
{
	class TransformNode2D : public Node
	{
	public:

		explicit TransformNode2D(const PuffinID& id = gInvalidID);
		~TransformNode2D() override = default;

		void begin_play() override;
		void update(double delta_time) override;
		void physics_update(double delta_time) override;
		void end_play() override;

		TransformComponent2D& get_transform();

	protected:

		

	};
}
