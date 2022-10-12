#pragma once

#include "ECS/ECS.h"

namespace Puffin::Physics
{
	struct CollisionEvent
	{
		ECS::EntityID entityA;
		ECS::EntityID entityB;

		bool operator<(const CollisionEvent& other) const
		{
			return (entityA < other.entityA && entityB < other.entityB) ||
					(entityA < other.entityB && entityB < other.entityA);
		}
	};

	typedef CollisionEvent CollisionBeginEvent;
	typedef CollisionEvent CollisionEndEvent;
}