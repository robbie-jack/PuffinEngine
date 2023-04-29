#pragma once

#include "Types/UUID.h"
#include <cstdint>

namespace Puffin::ECS
{
	typedef UUID EntityID;
	constexpr size_t MAX_ENTITIES = 50000;

	static const size_t INVALID_ENTITY = 0;
}