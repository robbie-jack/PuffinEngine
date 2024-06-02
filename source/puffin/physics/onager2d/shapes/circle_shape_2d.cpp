#include "puffin/physics/onager2d/shapes/circle_shape_2d.h"

#include "puffin/physics/onager2d/shapes/shape_2d.h"

namespace puffin::physics
{
	ShapeType2D CircleShape2D::getType() const
	{
		return ShapeType2D::circle;
	}

	AABB_2D CircleShape2D::getAABB(const Vector2f& position, const float& rotation) const
	{
		AABB_2D aabb;
		aabb.min = position - Vector2(radius, radius);
		aabb.max = position + Vector2(radius, radius);
		return aabb;
	}
}
