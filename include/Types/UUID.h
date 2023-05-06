#pragma once

#include <xhash>
#include <random>
#include "nlohmann/json.hpp"

namespace puffin
{
	static std::random_device randomDevice;
	static std::mt19937_64 randEngine(randomDevice());
	static std::uniform_int_distribution<uint64_t> uniformDistribution;

	using PuffinId = uint64_t;
	constexpr static PuffinId gInvalidId = 0;

	static PuffinId generateId()
	{
		return uniformDistribution(randEngine);
	}
}
