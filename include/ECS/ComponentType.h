#pragma once

#include <bitset>

namespace puffin::ECS
{
	typedef uint8_t ComponentSigType; // Type as it maps to signature
	const ComponentSigType MAX_COMPONENTS = 255;

	typedef std::bitset<MAX_COMPONENTS> Signature;
}