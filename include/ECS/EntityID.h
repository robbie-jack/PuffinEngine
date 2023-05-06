#pragma once

#include "Types/UUID.h"
#include <cstdint>

namespace puffin::ECS
{
	using EntityID = UUID;
	constexpr size_t MAX_ENTITIES = 50000;

	static const size_t INVALID_ENTITY = 0;
}