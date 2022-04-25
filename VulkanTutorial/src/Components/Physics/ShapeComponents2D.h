#pragma once

#ifndef SHAPE_COMPONENT_2D_H
#define SHAPE_COMPONENT_2D_H

#include "Physics/Shapes/CircleShape2D.h"
#include "Physics/Shapes/BoxShape2D.h"

#include <Types/Vector.h>

#include <memory>

namespace Puffin
{
	namespace Physics
	{
		struct CircleComponent2D
		{
			std::shared_ptr<CircleShape2D> shape;

			template<class Archive>
			void save(Archive& archive) const 
			{
				if (shape != nullptr)
					archive(*shape);
			}

			template<class Archive>
			void load(Archive& archive)
			{
				shape = std::make_shared<CircleShape2D>();

				archive(*shape);
			}
		};

		struct BoxComponent2D
		{
			std::shared_ptr<BoxShape2D> shape;

			template<class Archive>
			void save(Archive& archive) const
			{
				if (shape != nullptr)
					archive(*shape);
			}

			template<class Archive>
			void load(Archive& archive)
			{
				shape = std::make_shared<BoxShape2D>();

				archive(*shape);
			}
		};
	}
}

#endif //SHAPE_COMPONENT_2D_H