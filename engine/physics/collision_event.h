#pragma once

#include "types/uuid.h"

namespace puffin::physics
{
	struct CollisionBeginEvent
	{
		UUID entityA;
		UUID entityB;

		bool operator<(const CollisionBeginEvent& other) const
		{
			return (entityA < other.entityA && entityB < other.entityB) ||
					(entityA < other.entityB && entityB < other.entityA);
		}
	};

	struct CollisionEndEvent
	{
		UUID entityA;
		UUID entityB;

		bool operator<(const CollisionEndEvent& other) const
		{
			return (entityA < other.entityA&& entityB < other.entityB) ||
				(entityA < other.entityB&& entityB < other.entityA);
		}
	};
}