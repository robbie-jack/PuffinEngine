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
			Vector2 min;
			Vector2 max;
		};

		namespace Collision2D
		{
			struct Contact
			{
				Contact() {}

				ECS::Entity a, b; // Entities which collided

				Vector2 pointOnA, pointOnB;
				Vector2 normal;
				Float seperation;
			};
		}
	}
}

#endif // PHYSICS_TYPES_2D_H