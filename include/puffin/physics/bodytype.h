#pragma once

#include "nlohmann/json.hpp"

namespace puffin::physics
{
	enum class BodyType : uint8_t
	{
		Static = 0,
		Kinematic = 1,
		Dynamic = 2
	};

	inline const std::unordered_map<std::string, BodyType> gStringToBodyType =
	{
		{ "Static", BodyType::Static },
		{ "Kinematic", BodyType::Kinematic },
		{ "Dynamic", BodyType::Dynamic }
	};

	inline const std::unordered_map<BodyType, std::string> gBodyTypeToString =
	{
		{ BodyType::Static, "Static" },
		{ BodyType::Kinematic, "Kinematic" },
		{ BodyType::Dynamic, "Dynamic" }
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(BodyType,
	{
		{BodyType::Static, "Static"},
		{BodyType::Kinematic, "Kinematic"},
		{BodyType::Dynamic, "Dynamic"}
	})
}