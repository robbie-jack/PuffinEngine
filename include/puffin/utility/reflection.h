#pragma once

#include "entt/meta/factory.hpp"
#include "entt/meta/meta.hpp"

namespace entt
{
	using hs = hashed_string;
}

namespace puffin
{
	template<typename T>
	void RegisterType()
	{
		entt::meta<T>();
	};
}
