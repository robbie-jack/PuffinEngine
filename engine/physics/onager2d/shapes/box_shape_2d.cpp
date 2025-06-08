#include "physics/onager2d/shapes/box_shape_2d.h"

#include "physics/onager2d/shapes/shape_2d.h"
#include "physics/onager2d/physics_types_2d.h"
#include "types/aabb.h"

namespace puffin::physics
{
	ShapeType2D BoxShape2D::GetType() const
	{
		return ShapeType2D::Box;
	}

	AABB2D BoxShape2D::GetAABB(const Vector2f& position, const float& rotation) const
	{
		AABB2D aabb;
        aabb.min = position - half_extent;
        aabb.max = position + half_extent;
		return aabb;
	}

	void BoxShape2D::UpdatePoints()
	{
		points.clear();

        points.emplace_back(-half_extent.x, -half_extent.y);
        points.emplace_back(-half_extent.x, half_extent.y);
        points.emplace_back(half_extent.x, half_extent.y);
        points.emplace_back(half_extent.x, -half_extent.y);
	}
}
