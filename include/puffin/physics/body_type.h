#pragma once

#include "nlohmann/json.hpp"

namespace puffin::physics
{
	enum class BodyType
	{
		Static = 0,
		Kinematic = 1,
		Dynamic = 2
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(BodyType,
	{
		{BodyType::Static, "Static"},
		{BodyType::Kinematic, "Kinematic"},
		{BodyType::Dynamic, "Dynamic"}
	})
}