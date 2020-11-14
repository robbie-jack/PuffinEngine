#pragma once

#include "Vector.h"

namespace Puffin
{
	struct TransformComponent
	{
		Puffin::Vector3 position;
		Puffin::Vector3 rotation;
		Puffin::Vector3 scale;
	};
}
