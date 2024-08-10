#pragma once

#include "puffin/core/subsystem.h"

#include <unordered_map>
#include <memory>
#include <cassert>

namespace puffin::core
{
	class ISubsystemFactory
	{
	public:

		virtual ~ISubsystemFactory() = default;

		[[nodiscard]] virtual Subsystem* create(const std::shared_ptr<Engine>& engine) const = 0;

	};

	template<typename SubsystemT>
	class SubsystemFactory : public ISubsystemFactory
	{
	public:

		SubsystemFactory() {}
		~SubsystemFactory() override = default;

		[[nodiscard]] Subsystem* create(const std::shared_ptr<Engine>& engine) const final
		{
			return new SubsystemT(engine);
		}
	};

	class ISubsystemManager
	{
	public:

		virtual ~ISubsystemManager() = default;

		template<typename T>
		T* create_and_initialize_subsystem()
		{
			const char* type_name = typeid(T).name();
			auto subsystem = create_and_initialize_subsystem_internal(type_name);

			return static_cast<T*>(subsystem);

		}

	protected:

		virtual Subsystem* create_and_initialize_subsystem_internal(const char* type_name) = 0;

	};

	template<typename T>
	class SubsystemManager : public ISubsystemManager
	{
	public:

		SubsystemManager(const std::shared_ptr<Engine>& engine) : m_engine(engine) {}
		~SubsystemManager() override { m_engine = nullptr; }

		template<typename SubsystemT>
		void register_subsystem()
		{
			const char* type_name = typeid(SubsystemT).name();

			assert(m_subsystem_factories.find(type_name) == m_subsystem_factories.end() && "SubsystemManager::register_subsystem() - Attempting to register subsystem more than once");

			m_subsystem_factories.emplace(type_name, std::make_shared<SubsystemFactory<SubsystemT>>());
		}

		template<typename SubsystemT>
		T* get_subsystem()
		{
			const char* type_name = typeid(SubsystemT).name();

			if (m_subsystems.find(type_name) == m_subsystems.end())
			{
				return nullptr;
			}

			return m_subsystems.at(type_name);
		}

		void create_and_initialize_subsystems()
		{
			for (const auto& [type_name, subsystem_factory] : m_subsystem_factories)
			{
				create_and_initialize_subsystem_internal(type_name);
			}
		}

		void destroy_subsystems()
		{
			for (auto& subsystem_typed : m_subsystems_vector)
			{
				auto subsystem = static_cast<Subsystem*>(subsystem_typed);
				subsystem->deinitialize();
				delete subsystem_typed;
			}

			m_subsystems.clear();
			m_subsystems_vector.clear();
		}

		[[nodiscard]] std::vector<T*>& get_subsystems()
		{
			return m_subsystems_vector;
		}

	protected:

		Subsystem* create_and_initialize_subsystem_internal(const char* type_name) final
		{
			// Return if subsystem of this type is already created
			if (m_subsystems.find(type_name) != m_subsystems.end())
			{
				return m_subsystems.at(type_name);
			}

			assert(m_subsystem_factories.find(type_name) != m_subsystem_factories.end() && "SubsystemManager::create_and_initialize_subsystem_internal() - Attempting to create subsystem that wasn't registered");

			const auto& subsystem_factory = m_subsystem_factories.at(type_name);
			auto subsystem = subsystem_factory->create(m_engine);
			auto subsystem_typed = static_cast<T*>(subsystem);

			assert(subsystem_typed != nullptr && "SubsystemManager::create_and_initialize_subsystem_internal - Failed to cast created subsystem to type");

			subsystem->initialize(this);

			m_subsystems.emplace(type_name, subsystem);
			m_subsystems_vector.push_back(subsystem_typed);
			return subsystem;
		}

	private:

		std::shared_ptr<Engine> m_engine = nullptr;

		std::unordered_map<const char*, Subsystem*> m_subsystems;
		std::vector<T*> m_subsystems_vector;
		std::unordered_map<const char*, std::shared_ptr<ISubsystemFactory>> m_subsystem_factories;

	};
}
