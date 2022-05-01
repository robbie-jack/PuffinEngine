#pragma once

#include "ECS/ECS.h"

namespace Puffin::Physics
{
	struct CollisionEvent
	{
		ECS::Entity entityA;
		ECS::Entity entityB;
	};

	typedef CollisionEvent CollisionBeginEvent;
	typedef CollisionEvent CollisionEndEvent;
}