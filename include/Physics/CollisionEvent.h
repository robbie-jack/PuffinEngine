#pragma once

#include "Types/UUID.h"

namespace puffin::physics
{
	struct CollisionBeginEvent
	{
		PuffinId entityA;
		PuffinId entityB;

		bool operator<(const CollisionBeginEvent& other) const
		{
			return (entityA < other.entityA && entityB < other.entityB) ||
					(entityA < other.entityB && entityB < other.entityA);
		}
	};

	struct CollisionEndEvent
	{
		PuffinId entityA;
		PuffinId entityB;

		bool operator<(const CollisionEndEvent& other) const
		{
			return (entityA < other.entityA&& entityB < other.entityB) ||
				(entityA < other.entityB&& entityB < other.entityA);
		}
	};
}