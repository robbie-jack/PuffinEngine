#include "puffin/physics/onager2d/shapes/circleshape2d.h"

#include "puffin/physics/onager2d/shapes/shape2d.h"

namespace puffin::physics
{
	ShapeType2D CircleShape2D::GetType() const
	{
		return ShapeType2D::Circle;
	}

	AABB2D CircleShape2D::GetAABB(const Vector2f& position, const float& rotation) const
	{
		AABB2D aabb;
		aabb.min = position - Vector2(radius, radius);
		aabb.max = position + Vector2(radius, radius);
		return aabb;
	}
}
