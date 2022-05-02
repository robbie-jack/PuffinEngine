#pragma once

#include <cstdint>

namespace Puffin::ECS
{
	typedef uint32_t Entity;
	const Entity MAX_ENTITIES = 5000;

	static const Entity INVALID_ENTITY = 0;
}