#pragma once

#include <random>
#include <cstdint>

namespace puffin
{
	static std::random_device randomDevice;
	static std::mt19937_64 randEngine(randomDevice());
	static std::uniform_int_distribution<uint64_t> uniformDistribution;

    using UUID = uint_least64_t;
	constexpr static UUID gInvalidID = 0;

	static UUID GenerateId()
	{
		UUID id = gInvalidID;

		// Ensure that generated id will never equal invalid id (though this is extremely unlikely)
		while (id == gInvalidID)
		{
			id = uniformDistribution(randEngine);
		}

		return id;
	}
}
