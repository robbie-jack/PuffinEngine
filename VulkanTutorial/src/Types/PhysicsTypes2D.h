#pragma once

#ifndef PHYSICS_TYPES_2D_H
#define PHYSICS_TYPES_2D_H

namespace Puffin
{
	namespace Physics
	{
		namespace Collision2D
		{
			enum class ShapeType : uint8_t
			{
				NONE = 0,
				CIRCLE = 1,
				BOX = 2
			};

			struct Contact
			{
				Contact(ECS::Entity inA, ECS::Entity inB)
				{
					a = inA;
					b = inB;
				}

				ECS::Entity a, b; // Entities which collided

				Vector2 pointOnA, pointOnB;
				Vector2 normal;
				Float seperation;
			};
		}
	}
}

#endif // PHYSICS_TYPES_2D_H