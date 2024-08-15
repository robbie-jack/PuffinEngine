#pragma once

#include "puffin/core/subsystem.h"

#include <unordered_map>
#include <memory>
#include <cassert>

#include "puffin/core/engine.h"

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

	class SubsystemManager
	{
	public:

		explicit SubsystemManager(const std::shared_ptr<Engine>& engine);
		~SubsystemManager() { m_engine = nullptr; }

		template<typename T>
		void register_subsystem()
		{
			const char* type_name = typeid(T).name();

			// Add new subsystem factory
			assert(m_subsystem_factories.find(type_name) == m_subsystem_factories.end() && "SubsystemManager::register_subsystem() - Attempting to register subsystem more than once");

			m_subsystem_factories.emplace(type_name, new SubsystemFactory<T>());
			auto subsystem_factory = m_subsystem_factories.at(type_name);

			// Create uninitialized instance of subsystem
			auto subsystem = subsystem_factory->create(m_engine);
			m_subsystems.emplace(type_name, subsystem);

			if (subsystem->type() == SubsystemType::Input)
			{
				assert(m_registered_input_subsystem == false && "SubsystemManager::register_subsystem - Attempting to register a second input subsystem");

				m_registered_input_subsystem = true;
			}

			if (subsystem->type() == SubsystemType::Input)
			{
				assert(m_registered_render_subsystem == false && "SubsystemManager::register_subsystem - Attempting to register a second render subsystem");

				m_registered_render_subsystem = true;
			}

			if (is_editor_type(subsystem->type()))
			{
				m_engine_subsystem_names.push_back(type_name);
			}
			else
			{
				m_gameplay_subsystem_names.push_back(type_name);
			}
		}

		template<typename T>
		T* get_subsystem()
		{
			const char* type_name = typeid(T).name();

			assert(m_initialized_subsystems.find(type_name) != m_initialized_subsystems.end() && "SubsystemManager::get_subsystem() - Attempting to get subsystem which has not been initialized");

			return static_cast<T*>(m_initialized_subsystems.at(type_name));
		}

		std::vector<Subsystem*>& get_subsystems();
		std::vector<Subsystem*>& get_gameplay_subsystems();

		Subsystem* get_input_subsystem() const;
		Subsystem* get_render_subsystem() const;

		template<typename T>
		T* create_and_initialize_subsystem()
		{
			const char* type_name = typeid(T).name();
			auto subsystem = create_subsystem_internal(type_name);

			initialize_subsystem_internal(type_name);

			return static_cast<T*>(subsystem);
		}

		void create_and_initialize_engine_subsystems();
		void create_and_initialize_gameplay_subsystems();

		void destroy_engine_subsystems();
		void destroy_gameplay_subsystems();

	private:

		std::shared_ptr<Engine> m_engine = nullptr;

		// Array of all subsystems, initialized and uninitialized
		std::unordered_map<const char*, Subsystem*> m_subsystems;

		// Array of all initialized subsystems
		std::unordered_map<const char*, Subsystem*> m_initialized_subsystems;

		std::unordered_map<const char*, ISubsystemFactory*> m_subsystem_factories;

		std::vector<const char*> m_engine_subsystem_names;
		std::vector<Subsystem*> m_initialized_engine_subsystems;

		std::vector<const char*> m_gameplay_subsystem_names;
		std::vector<Subsystem*> m_initialized_gameplay_subsystems;

		bool m_registered_input_subsystem = false;
		Subsystem* m_input_subsystem = nullptr;

		bool m_registered_render_subsystem = false;
		Subsystem* m_render_subsystem = nullptr;

		static bool is_editor_type(SubsystemType type);
		static bool is_gameplay_type(SubsystemType type);

		Subsystem* create_subsystem_internal(const char* type_name);
		void initialize_subsystem_internal(const char* type_name);

		Subsystem* create_and_initialize_subsystem_internal(const char* type_name);
	};
}
