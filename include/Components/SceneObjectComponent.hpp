#pragma once

#include "Types/UUID.h"
#include "nlohmann/json.hpp"

#include <string>

namespace puffin
{
	// Stores data about scene objects such as uuid, name, etc...
	struct SceneObjectComponent
	{
		SceneObjectComponent() = default;
		SceneObjectComponent(const std::string& name_) : name(name_) {}

		UUID uuid; // uint64_t which is used to uniquely identify entities
		std::string name; // Name of entity, used for displaying in UI

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SceneObjectComponent, uuid, name)
	};
}
