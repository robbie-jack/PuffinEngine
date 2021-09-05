#pragma once

#ifndef SHAPE_COMPONENT_2D_H
#define SHAPE_COMPONENT_2D_H

#include <Types/PhysicsTypes2D.h>
#include <Types/Vector.h>

namespace Puffin
{
	namespace Physics
	{
		struct ShapeComponent2D
		{
			Vector2 centreOfMass;
		};

		struct CircleComponent2D : public ShapeComponent2D
		{
			CircleComponent2D()
			{
				centreOfMass.Zero();
				radius = 1.0f;
			}

			Float radius;
		};

		template<class Archive>
		void serialize(Archive& archive, CircleComponent2D& circle)
		{
			archive(circle.centreOfMass);
			archive(circle.radius);
		}

		struct BoxComponent2D : public ShapeComponent2D
		{
			BoxComponent2D()
			{
				centreOfMass.Zero();
				halfExtent = Vector2(.5f, .5f);
			}

			Vector2 halfExtent;
		};

		template<class Archive>
		void serialize(Archive& archive, BoxComponent2D& box)
		{
			archive(box.centreOfMass);
			archive(box.halfExtent);
		}
	}
}

#endif //SHAPE_COMPONENT_2D_H