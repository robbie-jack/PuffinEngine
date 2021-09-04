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
				CIRCLE = 0,
				BOX = 1
			};

			struct Shape
			{
				Vector2 centerOfMass;
			};

			struct ShapeCircle : Shape
			{
				ShapeCircle()
				{
					centerOfMass.Zero();
				}

				float radius;
			};

			template<class Archive>
			void serialize(Archive& archive, ShapeCircle& circle)
			{
				archive(circle.radius);
			}

			struct ShapeBox : Shape
			{
				Vector2 halfExtent;
			};

			template<class Archive>
			void serialize(Archive& archive, ShapeBox& box)
			{
				archive(box.halfExtent);
			}

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