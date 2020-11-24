#pragma once

#include "Vector.h"
#include "BaseComponent.h"

namespace Puffin
{
	struct TransformComponent : public BaseComponent
	{
		Puffin::Vector3 position;
		Puffin::Vector3 rotation;
		Puffin::Vector3 scale;
	};
}
