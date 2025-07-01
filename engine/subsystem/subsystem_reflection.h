#pragma once

#include <unordered_set>

#include "utility/reflection.h"

namespace puffin::core
{
	class Subsystem;
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

			if (m_registeredTypes.find(typeId) != m_registeredTypes.end())
				return;

			m_registeredTypes.insert(typeId);
			m_registeredTypesInOrder.push_back(typeId);
		}

		const std::unordered_set<entt::id_type>& GetRegisteredTypes() const
		{
			return m_registeredTypes;
		}

		const std::vector<entt::id_type>& GetRegisteredTypesInOrder() const
		{
			return m_registeredTypesInOrder;
		}

	private:

		std::unordered_set<entt::id_type> m_registeredTypes;
		std::vector<entt::id_type> m_registeredTypesInOrder;

	};

	template<typename T>
	core::Subsystem* CreateSubsystem(std::shared_ptr<core::Engine> engine)
	{
		return dynamic_cast<core::Subsystem*>(new T(engine));
	}

	/*
	 * Register subsystem to be instantiated and initialized at runtime,
	 * should only be used for subsystems which should have a live instance,
	 * not base classes like EngineSubsystem or GameplaySubsystem
	 */
	template<typename T>
	void RegisterSubsystemDefault(entt::meta_factory<T>& meta)
	{
		meta.func<&CreateSubsystem<T>>(entt::hs("CreateSubsystem"));

		SubsystemRegistry::Get()->Register<T>();
	}
}
