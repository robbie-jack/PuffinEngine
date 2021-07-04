#pragma once

#ifndef SHAPE_COMPONENT_2D_H
#define SHAPE_COMPONENT_2D_H

#include <Components/BaseComponent.h>
#include <Types/Vector.h>

namespace Puffin
{
	namespace Physics
	{
		enum class ShapeType : uint8_t
		{
			CIRCLE = 0,
			BOX = 1
		};

		struct ShapeCircle
		{
			float radius;
		};

		template<class Archive>
		void serialize(Archive& archive, ShapeCircle& circle)
		{
			archive(circle.radius);
		}

		struct ShapeBox
		{
			Vector2 halfExtent;
		};

		template<class Archive>
		void serialize(Archive& archive, ShapeBox& box)
		{
			archive(box.halfExtent);
		}

		struct ShapeComponent2D : public BaseComponent
		{
			ShapeComponent2D()
			{
				bFlagCreated = false;
				bFlagDeleted = false;
			}

			ShapeComponent2D(ShapeType inType) :
				type(inType)
			{
				ShapeComponent2D();
			}

			ShapeType type = ShapeType::CIRCLE; // Shape Type

			// Shapes
			ShapeCircle circle;
			ShapeBox box;
		};

		template<class Archive>
		void save(Archive& archive, const ShapeComponent2D& shape)
		{
			uint8_t shapeType = (uint32_t)shape.type;

			archive(shapeType);

			switch (shape.type)
			{
			case ShapeType::CIRCLE:
				archive(shape.circle);
				break;
			case ShapeType::BOX:
				archive(shape.box);
				break;
			}
		}

		template<class Archive>
		void load(Archive& archive, ShapeComponent2D& shape)
		{
			uint8_t shapeType;

			archive(shapeType);
			shape.type = (ShapeType)shapeType;

			switch (shape.type)
			{
			case ShapeType::CIRCLE:
				archive(shape.circle);
				break;
			case ShapeType::BOX:
				archive(shape.box);
				break;
			}
		}
	}
}

#endif //SHAPE_COMPONENT_2D_H