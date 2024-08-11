#include "puffin/core/subsystem_manager.h"

#include "puffin/core/engine.h"

namespace puffin::core
{
	SubsystemManager::SubsystemManager(const std::shared_ptr<Engine>& engine) : m_engine(engine)
	{
	}

	std::vector<Subsystem*>& SubsystemManager::get_subsystems()
	{
		return m_initialized_engine_subsystems;
	}

	std::vector<Subsystem*>& SubsystemManager::get_gameplay_subsystems()
	{
		return m_initialized_gameplay_subsystems;
	}

	Subsystem* SubsystemManager::get_input_subsystem() const
	{
		assert(m_input_subsystem != nullptr && "SubsystemManager::get_input_subsystem() - Attempting to get input subsystem while it is invalid");

		return m_input_subsystem;
	}

	Subsystem* SubsystemManager::get_render_subsystem() const
	{
		assert(m_render_subsystem != nullptr && "SubsystemManager::get_render_subsystem() - Attempting to get render subsystem while it is invalid");

		return m_render_subsystem;
	}

	void SubsystemManager::create_and_initialize_engine_subsystems()
	{
		for (const auto& type_name : m_engine_subsystem_names)
		{
			auto subsystem = create_and_initialize_subsystem_internal(type_name);

			if (m_initialized_subsystems.find(type_name) != m_initialized_subsystems.end())
			{
				m_initialized_engine_subsystems.push_back(subsystem);
			}

			if (subsystem->type() == SubsystemType::Input)
			{
				assert(m_input_subsystem == nullptr && "SubsystemManager::create_and_initialize_engine_subsystems - Attempting to initialize a second input subsystem");

				m_input_subsystem = subsystem;
			}

			if (subsystem->type() == SubsystemType::Render)
			{
				assert(m_render_subsystem == nullptr && "SubsystemManager::create_and_initialize_engine_subsystems - Attempting to initialize a second render subsystem");

				m_render_subsystem = subsystem;
			}
		}
	}

	void SubsystemManager::create_and_initialize_gameplay_subsystems()
	{
		for (const auto& type_name : m_gameplay_subsystem_names)
		{
			auto subsystem = create_and_initialize_subsystem_internal(type_name);

			if (m_initialized_subsystems.find(type_name) != m_initialized_subsystems.end())
			{
				m_initialized_gameplay_subsystems.push_back(subsystem);
			}
		}
	}

	void SubsystemManager::destroy_engine_subsystems()
	{
		for (auto subsystem : m_initialized_engine_subsystems)
		{
			subsystem->deinitialize();
			delete subsystem;
		}

		m_initialized_engine_subsystems.clear();

		for (const auto& type_name : m_engine_subsystem_names)
		{
			m_initialized_subsystems.erase(type_name);
			m_subsystems.erase(type_name);
		}
	}

	void SubsystemManager::destroy_gameplay_subsystems()
	{
		for (auto subsystem : m_initialized_gameplay_subsystems)
		{
			subsystem->deinitialize();
			delete subsystem;
		}

		m_initialized_gameplay_subsystems.clear();

		for (const auto& type_name : m_gameplay_subsystem_names)
		{
			m_initialized_subsystems.erase(type_name);
			m_subsystems.erase(type_name);
		}
	}

	bool SubsystemManager::is_editor_type(SubsystemType type)
	{
		if (type == SubsystemType::Gameplay)
			return false;

		return true;
	}

	bool SubsystemManager::is_gameplay_type(SubsystemType type)
	{
		if (type == SubsystemType::Gameplay)
			return true;

		return false;
	}

	Subsystem* SubsystemManager::create_subsystem_internal(const char* type_name)
	{
		// Return if subsystem of this type is already created
		if (m_subsystems.find(type_name) != m_subsystems.end())
		{
			return m_subsystems.at(type_name);
		}

		assert(m_subsystem_factories.find(type_name) != m_subsystem_factories.end() && "SubsystemManager::create_subsystem_internal() - Attempting to create subsystem that wasn't registered");

		const auto& subsystem_factory = m_subsystem_factories.at(type_name);

		auto subsystem = subsystem_factory->create(m_engine);
		m_subsystems.emplace(type_name, subsystem);

		return subsystem;
	}

	void SubsystemManager::initialize_subsystem_internal(const char* type_name)
	{
		if (m_initialized_subsystems.find(type_name) != m_initialized_subsystems.end())
		{
			return;
		}

		assert(m_subsystem_factories.find(type_name) != m_subsystem_factories.end() && "SubsystemManager::initialize_subsystem_internal() - Attempting to initialize subsystem that wasn't registered");
		assert(m_subsystems.find(type_name) != m_subsystems.end() && "SubsystemManager::initialize_subsystem_internal() - Attempting to initialize subsystem that hasn't been created yet");

		auto subsystem = m_subsystems.at(type_name);

		if (subsystem->type() == SubsystemType::Editor && !m_engine->should_render_editor_ui())
		{
			return;
		}

		subsystem->initialize(this);

		m_initialized_subsystems.emplace(type_name, subsystem);
	}

	
	Subsystem* SubsystemManager::create_and_initialize_subsystem_internal(const char* type_name)
	{
		auto subsystem = create_subsystem_internal(type_name);

		initialize_subsystem_internal(type_name);

		return subsystem;
	}
}
