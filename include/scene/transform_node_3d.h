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

		TransformNode3D() = default;

		void create() override;
		void update(double delta_time) override;
		void physics_update(double delta_time) override;
		void destroy() override;

		TransformComponent3D& get_transform();

	protected:



	};
}
