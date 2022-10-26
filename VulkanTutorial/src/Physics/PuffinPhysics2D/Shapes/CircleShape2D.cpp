
#include "Physics/PuffinPhysics2D/Shapes/CircleShape2D.h"
#include "Physics/PuffinPhysics2D/Shapes/Shape2D.h"
#include "Physics/PuffinPhysics2D/PhysicsTypes2D.h"

namespace Puffin::Physics
{
	ShapeType2D CircleShape2D::GetType() const
	{
		return ShapeType2D::Circle;
	}

	AABB CircleShape2D::GetAABB(const Vector2f& position, const float& rotation) const
	{
		AABB aabb;
		aabb.min = position - Vector2(radius, radius);
		aabb.max = position + Vector2(radius, radius);
		return aabb;
	}
}
