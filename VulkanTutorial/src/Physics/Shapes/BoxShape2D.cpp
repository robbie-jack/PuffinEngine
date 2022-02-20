#include "BoxShape2D.h"

#include "Physics/PhysicsHelpers2D.h"
#include "Physics/PhysicsTypes2D.h"

namespace Puffin::Physics
{
	ShapeType2D BoxShape2D::GetType() const
	{
		return ShapeType2D::Box;
	}

	AABB BoxShape2D::GetAABB(const TransformComponent& transform) const
	{
		AABB aabb;
		aabb.min = transform.position.GetXY() - halfExtent_;
		aabb.max = transform.position.GetXY() + halfExtent_;
		return aabb;
	}
}
