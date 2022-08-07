#pragma once

#include <cstdint>

namespace Puffin::ECS
{
	typedef uint32_t EntityID;
	const EntityID MAX_ENTITIES = 5000;

	static const EntityID INVALID_ENTITY = 0;
}