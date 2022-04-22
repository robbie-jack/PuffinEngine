#pragma once

#include "ECS/ECS.h"

#ifndef PHYSICS_TYPES_2D_H
#define PHYSICS_TYPES_2D_H

namespace Puffin
{
	namespace Physics
	{
		struct AABB
		{
			Vector2f min;
			Vector2f max;
		};

		namespace Collision2D
		{
			struct Contact
			{
				Contact() {}

				ECS::Entity a, b; // Entities which collided

				Vector2f pointOnA, pointOnB;
				Vector2f normal;
				float seperation;
			};
		}
	}
}

#endif // PHYSICS_TYPES_2D_H