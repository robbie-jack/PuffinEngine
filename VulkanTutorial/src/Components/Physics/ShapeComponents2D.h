#pragma once

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
			std::shared_ptr<CircleShape2D> shape = nullptr;

			CircleComponent2D() = default;
			CircleComponent2D(const CircleComponent2D& circle)
			{
				shape = circle.shape;
			}

			~CircleComponent2D()
			{
				shape = nullptr;
			}

			CircleComponent2D& operator=(const CircleComponent2D& circle) = default;
		};

		struct BoxComponent2D
		{
			std::shared_ptr<BoxShape2D> shape = nullptr;

			BoxComponent2D() = default;
			BoxComponent2D(const BoxComponent2D& box)
			{
				shape = box.shape;
			}

			~BoxComponent2D()
			{
				shape = nullptr;
			}

			BoxComponent2D& operator=(const BoxComponent2D& circle) = default;
		};
	}
}