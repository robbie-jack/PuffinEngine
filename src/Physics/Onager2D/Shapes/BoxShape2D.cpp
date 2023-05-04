#include "Physics/Onager2D/Shapes/BoxShape2D.h"

#include "Physics/Onager2D/PhysicsHelpers2D.h"
#include "Physics/Onager2D/PhysicsTypes2D.h"
#include "Physics/Onager2D/Shapes/Shape2D.h"

namespace puffin::physics
{
	ShapeType2D BoxShape2D::getType() const
	{
		return ShapeType2D::box;
	}

	AABB BoxShape2D::getAABB(const Vector2f& position, const float& rotation) const
	{
		AABB aabb;
		aabb.min = position - halfExtent;
		aabb.max = position + halfExtent;
		return aabb;
	}

	void BoxShape2D::updatePoints()
	{
		points.clear();

		points.emplace_back(-halfExtent.x, -halfExtent.y);
		points.emplace_back(-halfExtent.x, halfExtent.y);
		points.emplace_back(halfExtent.x, halfExtent.y);
		points.emplace_back(halfExtent.x, -halfExtent.y);
	}
}
