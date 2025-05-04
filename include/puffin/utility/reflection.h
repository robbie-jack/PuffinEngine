#pragma once

#include "entt/meta/factory.hpp"
#include "entt/meta/meta.hpp"

#include "string_view"

namespace entt
{
	using hs = hashed_string;
}

namespace puffin::reflection
{
	template<typename CompT>
	std::string_view GetTypeString() = delete;

	template<typename CompT>
	entt::hs GetTypeHashedString() = delete;

	template<typename T>
	void RegisterType() = delete;
}
