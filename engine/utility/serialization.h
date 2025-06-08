#pragma once

#include <string>
#include <unordered_map>
#include <cassert>
#include <cstdint>


#include "nlohmann/json.hpp"

#include "entt/meta/factory.hpp"
#include <entt/entity/registry.hpp>

namespace puffin::serialization
{
	class Archive;

	template<typename T>
	nlohmann::json Serialize(const T& data) = delete;

	template<typename T>
	T Deserialize(const nlohmann::json& json) = delete;
	
}