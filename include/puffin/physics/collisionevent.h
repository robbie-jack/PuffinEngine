#pragma once

#include "puffin/types/uuid.h"

namespace puffin::physics
{
	struct CollisionBeginEvent
	{
		PuffinID entityA;
		PuffinID entityB;

		bool operator<(const CollisionBeginEvent& other) const
		{
			return (entityA < other.entityA && entityB < other.entityB) ||
					(entityA < other.entityB && entityB < other.entityA);
		}
	};

	struct CollisionEndEvent
	{
		PuffinID entityA;
		PuffinID entityB;

		bool operator<(const CollisionEndEvent& other) const
		{
			return (entityA < other.entityA&& entityB < other.entityB) ||
				(entityA < other.entityB&& entityB < other.entityA);
		}
	};
}