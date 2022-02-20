#pragma once

#include <Components/TransformComponent.h>

#include <Types/Vector.h>

namespace Puffin::Physics
{
	enum class ShapeType2D : uint8_t
	{
		Box = 1,
		Circle = 2
	};

	struct AABB;

	struct Shape2D
	{
		Shape2D()
		{
			centreOfMass_.Zero();
		}

		virtual ~Shape2D() = default;

		virtual ShapeType2D GetType() const = 0;

		//virtual AABB GetAABB(const TransformComponent& transform) const = 0;

		Vector2 centreOfMass_; // Centre of mass for this shape
	};
}
