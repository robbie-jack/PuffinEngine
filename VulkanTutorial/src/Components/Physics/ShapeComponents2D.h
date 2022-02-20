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
			CircleShape2D* shape_;

			template<class Archive>
			void serialize(Archive& archive)
			{
				archive(*shape_);
			}
		};

		struct BoxComponent2D
		{
			BoxShape2D* shape_;

			template<class Archive>
			void serialize(Archive& archive)
			{
				archive(*shape_);
			}
		};
	}
}

#endif //SHAPE_COMPONENT_2D_H