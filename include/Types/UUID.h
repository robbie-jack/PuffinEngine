#pragma once

#include <xhash>
#include <random>
#include "nlohmann/json.hpp"

namespace puffin
{
	static std::random_device randomDevice;
	static std::mt19937_64 randEngine(randomDevice());
	static std::uniform_int_distribution<uint64_t> uniformDistribution;

	class UUID
	{
	public:

		UUID()
			: mUuid(uniformDistribution(randEngine))
		{
			
		}

		explicit UUID(const uint64_t uuid)
			: mUuid(uuid)
		{
			
		}

		UUID(const UUID&) = default;

		operator uint64_t() const { return mUuid; }

		void operator= (const size_t uuid)
		{
			mUuid = uuid;
		}

		bool operator< (const UUID uuid) const
		{
			return mUuid < static_cast<uint64_t>(uuid);
		}

	private:
		uint64_t mUuid;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(UUID, mUuid)
	};
}

template<>
struct std::hash<puffin::UUID>
{
	std::size_t operator()(const puffin::UUID& uuid) const noexcept
	{
		return hash<uint64_t>()(static_cast<uint64_t>(uuid));
	}
};
