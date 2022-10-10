#include "BoxShape2D.h"

#include "Physics/PhysicsHelpers2D.h"
#include "Physics/PhysicsTypes2D.h"

namespace Puffin::Physics
{
	ShapeType2D BoxShape2D::GetType() const
	{
		return ShapeType2D::Box;
	}

	AABB BoxShape2D::GetAABB(const Vector2f& position, const float& rotation) const
	{
		AABB aabb;
		aabb.min = position - halfExtent;
		aabb.max = position + halfExtent;
		return aabb;
	}

	void BoxShape2D::UpdatePoints()
	{
		points.clear();

		points.emplace_back(-halfExtent.x, -halfExtent.y);
		points.emplace_back(-halfExtent.x, halfExtent.y);
		points.emplace_back(halfExtent.x, halfExtent.y);
		points.emplace_back(halfExtent.x, -halfExtent.y);
	}
}
