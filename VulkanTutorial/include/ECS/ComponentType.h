#pragma once

#include <bitset>

namespace Puffin::ECS
{
	typedef uint8_t ComponentType;
	const ComponentType MAX_COMPONENTS = 255;

	typedef std::bitset<MAX_COMPONENTS> Signature;
}