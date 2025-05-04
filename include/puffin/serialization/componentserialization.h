#pragma once

#include <unordered_set>

#include <entt/entity/registry.hpp>
#include "nlohmann/json.hpp"

#include "puffin/utility/reflection.h"
#include "puffin/utility/serialization.h"

namespace puffin::serialization
{
	template<typename CompT>
	bool HasComponent(std::shared_ptr<entt::registry> registry, entt::entity entity)
	{
		return registry->any_of<CompT>(entity);
	}

	template<typename CompT>
	nlohmann::json SerializeFromRegistry(std::shared_ptr<entt::registry> registry, entt::entity entity)
	{
		if (registry->any_of<CompT>(entity))
		{
			return Serialize<CompT>(registry->get<CompT>(entity));
		}

		return nlohmann::json {};
	}

	template<typename CompT>
	CompT& DeserializeToRegistry(std::shared_ptr<entt::registry> registry, entt::entity entity, const nlohmann::json& json)
	{
		return registry->emplace_or_replace<CompT>(entity, Deserialize<CompT>(json));
	}

	class ComponentRegistry
	{
		static ComponentRegistry* sInstance;

		ComponentRegistry() {}

	public:

		~ComponentRegistry() = default;

		static ComponentRegistry* Get()
		{
			if (!sInstance)
				sInstance = new ComponentRegistry();

			return sInstance;
		}

		template<typename CompT>
		void Register()
		{
			auto type = entt::resolve<CompT>();
			auto typeID = type.id();

			if (mRegisteredTypes.find(typeID) == mRegisteredTypes.end())
			{
				mRegisteredTypes.insert(typeID);
				mRegisteredTypesVector.push_back(typeID);
			}
		}

		[[nodiscard]] const std::vector<entt::id_type>& GetRegisteredTypesVector() const
		{
			return mRegisteredTypesVector;
		}

	private:

		std::unordered_set<entt::id_type> mRegisteredTypes;
		std::vector<entt::id_type> mRegisteredTypesVector;

	};
}
