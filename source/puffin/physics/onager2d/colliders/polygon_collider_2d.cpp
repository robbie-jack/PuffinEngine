
#include "puffin/physics/onager2d/colliders/polygon_collider_2d.h"

#include "puffin/math_helpers.h"
#include "puffin/physics/onager2d/physics_types_2d.h"

namespace puffin::physics::collision2D
{
	AABB PolygonCollider2D::getAABB() const
	{
		return shape->getAABB(position, rotation);
	}

	Vector2f PolygonCollider2D::findFurthestPoint(Vector2f direction) const
	{
		Vector2f maxPoint = maths::rotate_point_around_origin(shape->points[0], rotation);
		float maxDistance = direction.dot(maxPoint);

		for (int i = 1; i < shape->points.size(); i++)
		{
			Vector2f point = maths::rotate_point_around_origin(shape->points[i], rotation);

			const float distance = direction.dot(point);

			if (distance > maxDistance)
			{
				maxDistance = distance;
				maxPoint = point;
			}
		}

		return maxPoint + position;
	}
}
