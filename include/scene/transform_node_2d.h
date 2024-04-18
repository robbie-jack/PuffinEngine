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

		TransformNode2D() = default;

		void create() override;
		void update(double delta_time) override;
		void physics_update(double delta_time) override;
		void destroy() override;

		TransformComponent2D& get_transform();

	protected:

		

	};
}
