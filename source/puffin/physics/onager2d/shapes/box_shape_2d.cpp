#include "puffin/physics/onager2d/shapes/box_shape_2d.h"

#include "puffin/physics/onager2d/shapes/shape_2d.h"
#include "puffin/physics/onager2d/physics_types_2d.h"

namespace puffin::physics
{
	ShapeType2D BoxShape2D::getType() const
	{
		return ShapeType2D::box;
	}

	AABB BoxShape2D::getAABB(const Vector2f& position, const float& rotation) const
	{
		AABB aabb;
        aabb.min = position - half_extent;
        aabb.max = position + half_extent;
		return aabb;
	}

	void BoxShape2D::updatePoints()
	{
		points.clear();

        points.emplace_back(-half_extent.x, -half_extent.y);
        points.emplace_back(-half_extent.x, half_extent.y);
        points.emplace_back(half_extent.x, half_extent.y);
        points.emplace_back(half_extent.x, -half_extent.y);
	}
}
