
#include "Physics/Shapes/CircleShape2D.h"
#include "Physics/PhysicsTypes2D.h"

namespace Puffin::Physics
{
	ShapeType2D CircleShape2D::GetType() const
	{
		return ShapeType2D::Circle;
	}

	AABB CircleShape2D::GetAABB(const TransformComponent& transform) const
	{
		AABB aabb;
		aabb.min = transform.position.GetXY() - Vector2(radius_, radius_);
		aabb.max = transform.position.GetXY() + Vector2(radius_, radius_);
		return aabb;
	}
}