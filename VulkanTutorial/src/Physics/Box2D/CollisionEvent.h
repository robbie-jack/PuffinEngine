#pragma once

#include "ECS/ECS.h"

namespace Puffin::Physics
{
	struct CollisionEvent
	{
		ECS::EntityID entityA;
		ECS::EntityID entityB;
	};

	typedef CollisionEvent CollisionBeginEvent;
	typedef CollisionEvent CollisionEndEvent;
}