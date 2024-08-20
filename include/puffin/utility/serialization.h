﻿#pragma once

#include <string>
#include <unordered_map>
#include <cassert>
#include <cstdint>

#include "nlohmann/json.hpp"

namespace puffin::serialization
{
	class Archive;

	template<typename T>
	void Serialize(const T& type, Archive& archive) = delete;

	template<typename T>
	void Deserialize(const Archive& archive, T& type) = delete;

	/*
	 * Archive class for serializing types (components, nodes, etc...)
	 */
	class Archive
	{
	public:

		void Serialize(const std::string& name, bool type);
		void Deserialize(const std::string& name, bool& type) const;

		void Serialize(const std::string& name, float type);
		void Deserialize(const std::string& name, float& type ) const;

		void Serialize(const std::string& name, double type);
		void Deserialize(const std::string& name, double& type) const;

		void Serialize(const std::string& name, const std::string& type);	
		void Deserialize(const std::string& name, std::string&& type) const;

		void Serialize(const std::string& name, int8_t type);
		void Deserialize(const std::string& name, int8_t& type) const;

		void Serialize(const std::string& name, int16_t type);
		void Deserialize(const std::string& name, int16_t& type) const;

		void Serialize(const std::string& name, int32_t type);
		void Deserialize(const std::string& name, int32_t& type) const;

		void Serialize(const std::string& name, int64_t type);
		void Deserialize(const std::string& name, int64_t& type) const;

		void Serialize(const std::string& name, uint8_t type);
		void Deserialize(const std::string& name, uint8_t& type) const;

		void Serialize(const std::string& name, uint16_t type);
		void Deserialize(const std::string& name, uint16_t& type) const;

		void Serialize(const std::string& name, uint32_t type);
		void Deserialize(const std::string& name, uint32_t& type) const;

		void Serialize(const std::string& name, uint64_t type);
		void Deserialize(const std::string& name, uint64_t& type) const;

		template<typename T>
		void Serialize(const std::string& name, T type)
		{
			Archive archive;

			Serialize<T>(type, archive);

			mArchives.emplace(name, archive);
		}

		template<typename T>
		T Deserialize(const std::string& name)
		{
			assert(mArchives.find(name) != mArchives.end() && "template T Archive::Deserialize - No property with that name in archive");

			T type;

			Deserialize(mArchives.at(name), type);

			return type;
		}

		void DumpToJson(nlohmann::json& json);
		void PopulateFromJson(const nlohmann::json& json);

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