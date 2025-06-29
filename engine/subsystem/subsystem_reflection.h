#pragma once

#include <unordered_set>
#include <unordered_map>

#include "utility/reflection.h"

namespace puffin::core
{
	class Engine;
}

namespace puffin::reflection
{
	class SubsystemRegistry
	{
		static SubsystemRegistry* s_instance;

		SubsystemRegistry() {}

	public:

		~SubsystemRegistry() = default;

		static SubsystemRegistry* Get()
		{
			if (!s_instance)
				s_instance = new SubsystemRegistry();

			return s_instance;
		}

		template<typename T>
		void Register()
		{
			auto type = entt::resolve<T>();
			auto typeId = type.id();

			if (m_registeredTypes.find(typeId) == m_registeredTypes.end())
			{
				m_registeredTypes.insert(typeId);
			}
		}

		const std::unordered_set<entt::id_type>& GetRegisteredTypes() const
		{
			return m_registeredTypes;
		}

	private:

		std::unordered_set<entt::id_type> m_registeredTypes;

	};

	template<typename T>
	void RegisterSubsystemDefault(entt::meta_factory<T>& meta)
	{
		auto* registry = SubsystemRegistry::Get();
		registry->Register<T>();
	}
}
