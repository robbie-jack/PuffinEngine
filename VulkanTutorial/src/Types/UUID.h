#pragma once

#include <xhash>
#include <random>

namespace Puffin
{
	static std::random_device RandomDevice;
	static std::mt19937_64 RandEngine(RandomDevice());
	static std::uniform_int_distribution<uint64_t> UniformDistribution;

	class UUID
	{
	public:

		UUID()
			: uuid(UniformDistribution(RandEngine))
		{
			
		}

		UUID(uint64_t inUuid)
			: uuid(inUuid)
		{
			
		}

		UUID(const UUID&) = default;

		operator uint64_t() const { return uuid; }

	private:
		uint64_t uuid;
	};
}

namespace std
{
	template<>
	struct hash<Puffin::UUID>
	{
		std::size_t operator()(const Puffin::UUID& uuid) const
		{
			return hash<uint64_t>()((uint64_t)uuid);
		}
	};
}