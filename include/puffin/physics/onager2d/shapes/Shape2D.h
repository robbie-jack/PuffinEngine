#pragma once

#include "puffin/types/vector2.h"
#include "puffin/physics/shapetype2d.h"

namespace puffin
{
	struct AABB2D;
};

namespace puffin::physics
{
	struct Shape2D
	{
		Shape2D()
		{
            centreOfMass.Zero();
		}

		virtual ~Shape2D() = default;

		virtual ShapeType2D GetType() const = 0;

		virtual AABB2D GetAABB(const Vector2f& position, const float& rotation) const = 0;

        Vector2f centreOfMass; // Centre of mass for this shape
	};
}
