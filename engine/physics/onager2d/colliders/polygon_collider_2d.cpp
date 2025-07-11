
#include "physics/onager2d/colliders/polygon_collider_2d.h"

#include "math_helpers.h"
#include "physics/onager2d/physics_types_2d.h"

namespace puffin::physics::collision2D
{
	AABB2D PolygonCollider2D::getAABB() const
	{
		return shape->GetAABB(position, rotation);
	}

	Vector2f PolygonCollider2D::findFurthestPoint(Vector2f direction) const
	{
		Vector2f maxPoint = maths::RotatePointAroundOrigin(shape->points[0], rotation);
		float maxDistance = direction.Dot(maxPoint);

		for (int i = 1; i < shape->points.size(); i++)
		{
			Vector2f point = maths::RotatePointAroundOrigin(shape->points[i], rotation);

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
