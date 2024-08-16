#pragma once

#include <random>
#include <cstdint>

namespace puffin
{
	static std::random_device randomDevice;
	static std::mt19937_64 randEngine(randomDevice());
	static std::uniform_int_distribution<uint64_t> uniformDistribution;

    using UUID = uint_least64_t;
	constexpr static UUID gInvalidId = 0;

	static UUID GenerateId()
	{
		UUID id = gInvalidId;

		// Ensure that generated id will never equal invalid id (though this is extremely unlikely)
		while (id == gInvalidId)
		{
			id = uniformDistribution(randEngine);
		}

		return id;
	}
}
