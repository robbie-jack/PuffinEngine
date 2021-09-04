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
			ShapeComponent2D() {}

			ShapeComponent2D(Collision2D::ShapeType inType) :
				type(inType)
			{
			}

			Collision2D::ShapeType type = Collision2D::ShapeType::CIRCLE; // Shape Type

			// Shapes
			Collision2D::ShapeCircle circle;
			Collision2D::ShapeBox box;
		};

		template<class Archive>
		void save(Archive& archive, const ShapeComponent2D& shape)
		{
			uint8_t shapeType = (uint32_t)shape.type;

			archive(shapeType);

			switch (shape.type)
			{
			case Collision2D::ShapeType::CIRCLE:
				archive(shape.circle);
				break;
			case Collision2D::ShapeType::BOX:
				archive(shape.box);
				break;
			}
		}

		template<class Archive>
		void load(Archive& archive, ShapeComponent2D& shape)
		{
			uint8_t shapeType;

			archive(shapeType);
			shape.type = (Collision2D::ShapeType)shapeType;

			switch (shape.type)
			{
			case Collision2D::ShapeType::CIRCLE:
				archive(shape.circle);
				break;
			case Collision2D::ShapeType::BOX:
				archive(shape.box);
				break;
			}
		}
	}
}

#endif //SHAPE_COMPONENT_2D_H