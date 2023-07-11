#pragma once

//#include <xhash>
#include <random>
#include "nlohmann/json.hpp"

namespace puffin
{
	static std::random_device randomDevice;
	static std::mt19937_64 randEngine(randomDevice());
	static std::uniform_int_distribution<uint64_t> uniformDistribution;

	using PuffinID = uint64_t;
	constexpr static PuffinID gInvalidID = 0;

	static PuffinID generateID()
	{
		return uniformDistribution(randEngine);
	}
}
