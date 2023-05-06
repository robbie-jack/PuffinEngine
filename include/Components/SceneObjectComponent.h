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
		SceneObjectComponent(const PuffinID& id_, const std::string& name_) : id(id_), name(name_) {}

		PuffinID id; // uint64_t which is used to uniquely identify entities
		std::string name; // Name of entity, used for displaying in UI

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SceneObjectComponent, id, name)
	};
}
