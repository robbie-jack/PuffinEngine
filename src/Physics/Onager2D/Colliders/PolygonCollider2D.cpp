
#include "Physics/Onager2D/Colliders/PolygonCollider2D.h"
#include "Physics/Onager2D/PhysicsTypes2D.h"
#include "MathHelpers.h"

namespace puffin::physics::collision2D
{
	AABB PolygonCollider2D::getAABB() const
	{
		return shape->getAABB(position, rotation);
	}

	Vector2f PolygonCollider2D::findFurthestPoint(Vector2f direction) const
	{
		Vector2f maxPoint = Maths::RotatePointAroundOrigin(shape->points[0], rotation);
		float maxDistance = direction.Dot(maxPoint);

		for (int i = 1; i < shape->points.size(); i++)
		{
			Vector2f point = Maths::RotatePointAroundOrigin(shape->points[i], rotation);

			const float distance = direction.Dot(point);

			if (distance > maxDistance)
			{
				maxDistance = distance;
				maxPoint = point;
			}
		}

		return maxPoint + position;
	}
}
