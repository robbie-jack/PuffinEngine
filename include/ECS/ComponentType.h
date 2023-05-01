#pragma once

#include <bitset>

namespace Puffin::ECS
{
	typedef uint8_t ComponentSigType; // Type as it maps to signature
	const ComponentSigType MAX_COMPONENTS = 255;

	typedef std::bitset<MAX_COMPONENTS> Signature;
}