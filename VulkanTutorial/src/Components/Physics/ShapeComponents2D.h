#pragma once

#ifndef SHAPE_COMPONENT_2D_H
#define SHAPE_COMPONENT_2D_H

#include "Physics/Shapes/CircleShape2D.h"
#include "Physics/Shapes/BoxShape2D.h"
#include <Types/Vector.h>

namespace Puffin
{
	namespace Physics
	{
		struct CircleComponent2D
		{
			CircleShape2D* shape;

			template<class Archive>
			void save(Archive& archive) const 
			{
				if (shape != nullptr)
					archive(*shape);
			}

			template<class Archive>
			void load(Archive& archive)
			{
				shape = new CircleShape2D();

				archive(*shape);
			}
		};

		struct BoxComponent2D
		{
			BoxShape2D* shape;

			template<class Archive>
			void save(Archive& archive) const
			{
				if (shape != nullptr)
					archive(*shape);
			}

			template<class Archive>
			void load(Archive& archive)
			{
				shape = new BoxShape2D();

				archive(*shape);
			}
		};
	}
}

#endif //SHAPE_COMPONENT_2D_H