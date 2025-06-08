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

	template<typename CompT>
	void RegisterType() = delete;

	/*
	 * Register default values for type
	 */
	template<typename CompT>
	void RegisterTypeDefaults(entt::meta_factory<CompT>& meta)
	{
		meta.type(GetTypeHashedString<CompT>());
		meta.func<&GetTypeString<CompT>>(entt::hs("GetTypeString"));
		meta.func<&GetTypeHashedString<CompT>>(entt::hs("GetTypeHashedString"));
	}
}
