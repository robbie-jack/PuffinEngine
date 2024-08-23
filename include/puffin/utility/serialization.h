#pragma once

#include <string>
#include <unordered_map>
#include <cassert>
#include <cstdint>

#include "nlohmann/json.hpp"

#include "entt/meta/factory.hpp"

namespace puffin::serialization
{
	class Archive;

	template<typename T>
	void Serialize(const T& data, Archive& archive) = delete;

	template<typename T>
	void Deserialize(const Archive& archive, T& data) = delete;

	/*
	 * Archive class for serializing types (components, nodes, etc...)
	 */
	class Archive
	{
	public:

		~Archive();

		template<typename T>
		void Set(const std::string& name, const T& data)
		{
			Archive archive;

			Serialize<T>(data, archive);

			mArchives.emplace(name, archive);
		}

		template<typename T>
		void Get(const std::string& name, T& data) const
		{
			assert(mArchives.find(name) != mArchives.end() && "template T Archive::Deserialize - No property with that name in archive");

			Deserialize<T>(mArchives.at(name), data);
		}

		void Set(const std::string& name, bool data);
		void Get(const std::string& name, bool& data) const;

		void Set(const std::string& name, float data);
		void Get(const std::string& name, float& data) const;

		void Set(const std::string& name, double data);
		void Get(const std::string& name, double& data) const;

		void Set(const std::string& name, const std::string& data);	
		void Get(const std::string& name, std::string& data) const;

		void Set(const std::string& name, int8_t data);
		void Get(const std::string& name, int8_t& data) const;

		void Set(const std::string& name, int16_t data);
		void Get(const std::string& name, int16_t& data) const;

		void Set(const std::string& name, int32_t data);
		void Get(const std::string& name, int32_t& data) const;

		void Set(const std::string& name, int64_t data);
		void Get(const std::string& name, int64_t& data) const;

		void Set(const std::string& name, uint8_t data);
		void Get(const std::string& name, uint8_t& data) const;

		void Set(const std::string& name, uint16_t data);
		void Get(const std::string& name, uint16_t& data) const;

		void Set(const std::string& name, uint32_t data);
		void Get(const std::string& name, uint32_t& data) const;

		void Set(const std::string& name, uint64_t data);
		void Get(const std::string& name, uint64_t& data) const;

		void DumpToJson(nlohmann::json& json) const;
		void PopulateFromJson(const nlohmann::json& json);

		void Clear();

	private:

		std::unordered_map<std::string, bool> mBooleans;
		std::unordered_map<std::string, float> mFloats;
		std::unordered_map<std::string, double> mDoubles;
		
		std::unordered_map<std::string, std::string> mStrings;

		std::unordered_map<std::string, int8_t> mIntegersS8;
		std::unordered_map<std::string, int16_t> mIntegersS16;
		std::unordered_map<std::string, int32_t> mIntegersS32;
		std::unordered_map<std::string, int64_t> mIntegersS64;

		std::unordered_map<std::string, uint8_t> mIntegersU8;
		std::unordered_map<std::string, uint16_t> mIntegersU16;
		std::unordered_map<std::string, uint32_t> mIntegersU32;
		std::unordered_map<std::string, uint64_t> mIntegersU64;

		std::unordered_map<std::string, Archive> mArchives;

	};

	
}