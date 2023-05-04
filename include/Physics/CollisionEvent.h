#pragma once

#include "ECS/ECS.h"

namespace puffin::physics
{
	struct CollisionBeginEvent
	{
		ECS::EntityID entityA;
		ECS::EntityID entityB;

		bool operator<(const CollisionBeginEvent& other) const
		{
			return (entityA < other.entityA && entityB < other.entityB) ||
					(entityA < other.entityB && entityB < other.entityA);
		}
	};

	struct CollisionEndEvent
	{
		ECS::EntityID entityA;
		ECS::EntityID entityB;

		bool operator<(const CollisionEndEvent& other) const
		{
			return (entityA < other.entityA&& entityB < other.entityB) ||
				(entityA < other.entityB&& entityB < other.entityA);
		}
	};
}