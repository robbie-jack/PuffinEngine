#include "PolygonCollider2D.h"

#include "MathHelpers.h"

namespace Puffin::Physics::Collision2D
{
	AABB PolygonCollider2D::GetAABB() const
	{
		return shape->GetAABB(transform);
	}

	Vector2f PolygonCollider2D::FindFurthestPoint(Vector2f direction) const
	{
		//Vector2f maxPoint = shape->points[0];
		Vector2f maxPoint = Maths::RotatePointAroundOrigin(shape->points[0], transform.rotation.z);
		float maxDISTANCE = direction.Dot(maxPoint);

		for (int i = 1; i < shape->points.size(); i++)
		{
			Vector2f point = Maths::RotatePointAroundOrigin(shape->points[i], transform.rotation.z);

			float distance = direction.Dot(point);

			if (distance > maxDISTANCE)
			{
				maxDISTANCE = distance;
				maxPoint = point;
			}
		}

		return maxPoint + transform.position.GetXY();
	}
}