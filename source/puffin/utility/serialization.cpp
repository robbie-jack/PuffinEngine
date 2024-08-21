#include "puffin/utility/serialization.h"

#include <cassert>

namespace puffin::serialization
{
	void Archive::Set(const std::string& name, bool data)
	{
		mBooleans.emplace(name, data);
	}

	void Archive::Get(const std::string& name, bool& data) const
	{
		assert(mBooleans.find(name) != mBooleans.end() && "Archive::Deserialize(bool) - No property with that name in archive");

		data = mBooleans.at(name);
	}

	void Archive::Set(const std::string& name, float data)
	{
		mFloats.emplace(name, data);
	}

	void Archive::Get(const std::string& name, float& data) const
	{
		assert(mFloats.find(name) != mFloats.end() && "Archive::Deserialize(float) - No property with that name in archive");

		data = mFloats.at(name);
	}

	void Archive::Set(const std::string& name, double data)
	{
		mDoubles.emplace(name, data);
	}

	void Archive::Get(const std::string& name, double& data) const
	{
		assert(mDoubles.find(name) != mDoubles.end() && "Archive::Deserialize(double) - No property with that name in archive");

		data = mDoubles.at(name);
	}

	void Archive::Set(const std::string& name, const std::string& data)
	{
		mStrings.emplace(name, data);
	}

	void Archive::Get(const std::string& name, std::string& data) const
	{
		assert(mStrings.find(name) != mStrings.end() && "Archive::Deserialize(string) - No property with that name in archive");

		data = mStrings.at(name);
	}

	void Archive::Set(const std::string& name, int8_t data)
	{
		mIntegersS8.emplace(name, data);
	}

	void Archive::Get(const std::string& name, int8_t& data) const
	{
		assert(mIntegersS8.find(name) != mIntegersS8.end() && "Archive::Deserialize(int8) - No property with that name in archive");

		data = mIntegersS8.at(name);
	}

	void Archive::Set(const std::string& name, int16_t data)
	{
		mIntegersS16.emplace(name, data);
	}

	void Archive::Get(const std::string& name, int16_t& data) const
	{
		assert(mIntegersS16.find(name) != mIntegersS16.end() && "Archive::Deserialize(int16) - No property with that name in archive");

		data = mIntegersS16.at(name);
	}

	void Archive::Set(const std::string& name, int data)
	{
		mIntegersS32.emplace(name, data);
	}

	void Archive::Get(const std::string& name, int& data) const
	{
		assert(mIntegersS32.find(name) != mIntegersS32.end() && "Archive::Deserialize(int32) - No property with that name in archive");

		data = mIntegersS32.at(name);
	}

	void Archive::Set(const std::string& name, int64_t data)
	{
		mIntegersS64.emplace(name, data);
	}

	void Archive::Get(const std::string& name, int64_t& data) const
	{
		assert(mIntegersS64.find(name) != mIntegersS64.end() && "Archive::Deserialize(int64) - No property with that name in archive");

		data = mIntegersS64.at(name);
	}

	void Archive::Set(const std::string& name, uint8_t data)
	{
		mIntegersU8.emplace(name, data);
	}

	void Archive::Get(const std::string& name, uint8_t& data) const
	{
		assert(mIntegersU8.find(name) != mIntegersU8.end() && "Archive::Deserialize(uint8) - No property with that name in archive");

		data = mIntegersU8.at(name);
	}

	void Archive::Set(const std::string& name, uint16_t data)
	{
		mIntegersU16.emplace(name, data);
	}

	void Archive::Get(const std::string& name, uint16_t& data) const
	{
		assert(mIntegersU16.find(name) != mIntegersU16.end() && "Archive::Deserialize(uint16) - No property with that name in archive");

		data = mIntegersU16.at(name);
	}

	void Archive::Set(const std::string& name, uint32_t data)
	{
		mIntegersU32.emplace(name, data);
	}

	void Archive::Get(const std::string& name, uint32_t& data) const
	{
		assert(mIntegersU32.find(name) != mIntegersU32.end() && "Archive::Deserialize(uint32) - No property with that name in archive");

		data = mIntegersU32.at(name);
	}

	void Archive::Set(const std::string& name, uint64_t data)
	{
		mIntegersU64.emplace(name, data);
	}

	void Archive::Get(const std::string& name, uint64_t& data) const
	{
		assert(mIntegersU64.find(name) != mIntegersU64.end() && "Archive::Deserialize(uint64) - No property with that name in archive");

		data = mIntegersU64.at(name);
	}

	void Archive::DumpToJson(nlohmann::json& json) const
	{
		if (!mBooleans.empty())
			json["bool"] = mBooleans;

		if (!mFloats.empty())
			json["float"] = mFloats;

		if (!mDoubles.empty())
			json["double"] = mDoubles;

		if (!mStrings.empty())
			json["string"] = mStrings;

		if (!mIntegersS8.empty())
			json["intS8"] = mIntegersS8;

		if (!mIntegersS16.empty())
			json["intS16"] = mIntegersS16;

		if (!mIntegersS32.empty())
			json["intS32"] = mIntegersS32;

		if (!mIntegersS64.empty())
			json["intS64"] = mIntegersS64;

		if (!mIntegersU8.empty())
			json["intU8"] = mIntegersU8;

		if (!mIntegersU16.empty())
			json["intU16"] = mIntegersU16;

		if (!mIntegersU32.empty())
			json["intU32"] = mIntegersU32;

		if (!mIntegersU64.empty())
			json["intU64"] = mIntegersU64;

		if (!mArchives.empty())
		{
			std::unordered_map<std::string, nlohmann::json> archiveDumps;
			for (const auto& [name, archive] : mArchives)
			{
				nlohmann::json archiveJson;
				archive.DumpToJson(archiveJson);

				archiveDumps.emplace(name, archiveJson);
			}

			json["archive"] = archiveDumps;
		}
	}

	void Archive::PopulateFromJson(const nlohmann::json& json)
	{
		if (json.contains("bool"))
			mBooleans = json.at("bool");

		if (json.contains("float"))
			mFloats = json.at("float");

		if (json.contains("double"))
			mDoubles = json.at("double");

		if (json.contains("string"))
			mStrings = json.at("string");

		if (json.contains("intS8"))
			mStrings = json.at("intS8");

		if (json.contains("intS16"))
			mStrings = json.at("intS16");

		if (json.contains("intS32"))
			mStrings = json.at("intS32");

		if (json.contains("intS64"))
			mStrings = json.at("intS64");

		if (json.contains("intU8"))
			mStrings = json.at("intU8");

		if (json.contains("intU16"))
			mStrings = json.at("intU16");

		if (json.contains("intU32"))
			mStrings = json.at("intU32");

		if (json.contains("intU64"))
			mStrings = json.at("intU64");

		if (json.contains("archive"))
		{
			for (const auto& [name, archiveJson] : json.at("archive").items())
			{
				Archive archive;

				archive.PopulateFromJson(archiveJson);

				mArchives.emplace(name, archive);
			}
		}
	}
}
