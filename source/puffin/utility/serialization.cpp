#include "puffin/utility/serialization.h"

#include <cassert>

namespace puffin::serialization
{
	void Archive::Serialize(const std::string& name, bool type)
	{
		mBooleans.emplace(name, type);
	}

	void Archive::Deserialize(const std::string& name, bool& type) const
	{
		assert(mBooleans.find(name) != mBooleans.end() && "Archive::Deserialize(bool) - No property with that name in archive");

		type = mBooleans.at(name);
	}

	void Archive::Serialize(const std::string& name, float type)
	{
		mFloats.emplace(name, type);
	}

	void Archive::Deserialize(const std::string& name, float& type) const
	{
		assert(mFloats.find(name) != mFloats.end() && "Archive::Deserialize(float) - No property with that name in archive");

		type = mFloats.at(name);
	}

	void Archive::Serialize(const std::string& name, double type)
	{
		mDoubles.emplace(name, type);
	}

	void Archive::Deserialize(const std::string& name, double& type) const
	{
		assert(mDoubles.find(name) != mDoubles.end() && "Archive::Deserialize(double) - No property with that name in archive");

		type = mDoubles.at(name);
	}

	void Archive::Serialize(const std::string& name, const std::string& type)
	{
		mStrings.emplace(name, type);
	}

	void Archive::Deserialize(const std::string& name, std::string&& type) const
	{
		assert(mStrings.find(name) != mStrings.end() && "Archive::Deserialize(string) - No property with that name in archive");

		type = mStrings.at(name);
	}

	void Archive::Serialize(const std::string& name, int8_t type)
	{
		mIntegersS8.emplace(name, type);
	}

	void Archive::Deserialize(const std::string& name, int8_t& type) const
	{
		assert(mIntegersS8.find(name) != mIntegersS8.end() && "Archive::Deserialize(int8) - No property with that name in archive");

		type = mIntegersS8.at(name);
	}

	void Archive::Serialize(const std::string& name, int16_t type)
	{
		mIntegersS16.emplace(name, type);
	}

	void Archive::Deserialize(const std::string& name, int16_t& type) const
	{
		assert(mIntegersS16.find(name) != mIntegersS16.end() && "Archive::Deserialize(int16) - No property with that name in archive");

		type = mIntegersS16.at(name);
	}

	void Archive::Serialize(const std::string& name, int type)
	{
		mIntegersS32.emplace(name, type);
	}

	void Archive::Deserialize(const std::string& name, int& type) const
	{
		assert(mIntegersS32.find(name) != mIntegersS32.end() && "Archive::Deserialize(int32) - No property with that name in archive");

		type = mIntegersS32.at(name);
	}

	void Archive::Serialize(const std::string& name, int64_t type)
	{
		mIntegersS64.emplace(name, type);
	}

	void Archive::Deserialize(const std::string& name, int64_t& type) const
	{
		assert(mIntegersS64.find(name) != mIntegersS64.end() && "Archive::Deserialize(int64) - No property with that name in archive");

		type = mIntegersS64.at(name);
	}

	void Archive::Serialize(const std::string& name, uint8_t type)
	{
		mIntegersU8.emplace(name, type);
	}

	void Archive::Deserialize(const std::string& name, uint8_t& type) const
	{
		assert(mIntegersU8.find(name) != mIntegersU8.end() && "Archive::Deserialize(uint8) - No property with that name in archive");

		type = mIntegersU8.at(name);
	}

	void Archive::Serialize(const std::string& name, uint16_t type)
	{
		mIntegersU16.emplace(name, type);
	}

	void Archive::Deserialize(const std::string& name, uint16_t& type) const
	{
		assert(mIntegersU16.find(name) != mIntegersU16.end() && "Archive::Deserialize(uint16) - No property with that name in archive");

		type = mIntegersU16.at(name);
	}

	void Archive::Serialize(const std::string& name, uint32_t type)
	{
		mIntegersU32.emplace(name, type);
	}

	void Archive::Deserialize(const std::string& name, uint32_t& type) const
	{
		assert(mIntegersU32.find(name) != mIntegersU32.end() && "Archive::Deserialize(uint32) - No property with that name in archive");

		type = mIntegersU32.at(name);
	}

	void Archive::Serialize(const std::string& name, uint64_t type)
	{
		mIntegersU64.emplace(name, type);
	}

	void Archive::Deserialize(const std::string& name, uint64_t& type) const
	{
		assert(mIntegersU64.find(name) != mIntegersU64.end() && "Archive::Deserialize(uint64) - No property with that name in archive");

		type = mIntegersU64.at(name);
	}

	void Archive::DumpToJson(nlohmann::json& json)
	{

	}

	void Archive::PopulateFromJson(const nlohmann::json& json)
	{

	}
}
