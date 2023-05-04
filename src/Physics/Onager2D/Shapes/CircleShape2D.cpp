
#include "Physics/Onager2D/Shapes/CircleShape2D.h"
#include "Physics/Onager2D/Shapes/Shape2D.h"
#include "Physics/Onager2D/PhysicsTypes2D.h"

namespace puffin::physics
{
	ShapeType2D CircleShape2D::getType() const
	{
		return ShapeType2D::circle;
	}

	AABB CircleShape2D::getAABB(const Vector2f& position, const float& rotation) const
	{
		AABB aabb;
		aabb.min = position - Vector2(radius, radius);
		aabb.max = position + Vector2(radius, radius);
		return aabb;
	}
}
