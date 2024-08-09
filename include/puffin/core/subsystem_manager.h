#pragma once

#include "puffin/core/subsystem.h"
#include "puffin/core/engine_subsystem.h"
#include "puffin/editor/editor_subsystem.h"
#include "puffin/gameplay/gameplay_subsystem.h"
#include "puffin/physics/physics_subsystem.h"
#include "puffin/rendering/render_subsystem.h"

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

	class ISubsystemArray
	{
	public:

		virtual ~ISubsystemArray() = default;

		virtual void create_and_initialize_subsystems(const std::shared_ptr<Engine>& engine) = 0;
		virtual void destroy_subsystems() = 0;
	};

	template<typename T>
	class SubsystemArray : public ISubsystemArray
	{
	public:

		SubsystemArray() {}
		~SubsystemArray() override = default;

		template<typename SubsystemT>
		void register_subsystem()
		{
			const char* type_name = typeid(SubsystemT).name();

			assert(m_subsystem_factories.find(type_name) == m_subsystem_factories.end() && "SubsystemArray::register_subsystem() - Attempting to register subsystem more than once");

			m_subsystem_factories.emplace(type_name, std::make_shared<SubsystemFactory<SubsystemT>>());
		}

		void create_and_initialize_subsystems(const std::shared_ptr<Engine>& engine) final
		{
			for (const auto& [type_name, subsystem_factory] : m_subsystem_factories)
			{
				auto subsystem = subsystem_factory->create(engine);
				auto subsystem_typed = static_cast<T*>(subsystem);

				assert(subsystem_typed != nullptr && "SubsystemArray::create_and_initialize_subsystems - Failed to cast created subsystem to type");

				subsystem->initialize();

				m_subsystems.emplace(type_name, subsystem_typed);
				m_subsystems_vector.push_back(subsystem_typed);
			}
		}

		void destroy_subsystems() final
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

		[[nodiscard]] std::vector<T*>& get_systems() const
		{
			return m_subsystems_vector;
		}

	private:

		std::unordered_map<const char*, T*> m_subsystems;
		std::vector<T*> m_subsystems_vector;

		std::unordered_map<const char*, std::shared_ptr<ISubsystemFactory>> m_subsystem_factories;

	};

	class SubsystemManager
	{
	public:

		SubsystemManager(const std::shared_ptr<Engine>& engine)
		{
			add_subsystem_array<EngineSubsystem>();
			add_subsystem_array<editor::EditorSubsystem>();
			add_subsystem_array<gameplay::GameplaySubsystem>();
			add_subsystem_array<physics::PhysicsSubsystem>();
			add_subsystem_array<rendering::RenderSubsystem>();
		}

		template<typename T>
		void register_engine_subsystem()
		{
			auto subsystem_array = get_subsystem_array<EngineSubsystem>();

			subsystem_array.register_subsystem<T>();
		}

		template<typename T>
		void register_editor_subsystem()
		{
			auto subsystem_array = get_subsystem_array<editor::EditorSubsystem>();

			subsystem_array.register_subsystem<T>();
		}

		template<typename T>
		void register_gameplay_subsystem()
		{
			auto subsystem_array = get_subsystem_array<gameplay::GameplaySubsystem>();

			subsystem_array.register_subsystem<T>();
		}

		template<typename T>
		void register_physics_subsystem()
		{
			auto subsystem_array = get_subsystem_array<physics::PhysicsSubsystem>();

			subsystem_array.register_subsystem<T>();
		}

		template<typename T>
		void register_render_subsystem()
		{
			auto subsystem_array = get_subsystem_array<rendering::RenderSubsystem>();

			subsystem_array.register_subsystem<T>();
		}

		void create_and_initialize_subsystems()
		{
			for (const auto& [type_name, subsystem_array] : m_subsystem_arrays)
			{
				subsystem_array->create_and_initialize_subsystems(m_engine);
			}
		}

		void destroy_subsystems()
		{
			for (const auto& [type_name, subsystem_array] : m_subsystem_arrays)
			{
				subsystem_array->destroy_subsystems();
			}
		}

		template<typename T>
		[[nodiscard]] std::vector<T*>& get_subsystems()
		{
			return get_subsystem_array<T>().get_systems();
		}

	private:

		std::shared_ptr<Engine> m_engine = nullptr;

		std::unordered_map<const char*, std::shared_ptr<ISubsystemArray>> m_subsystem_arrays;

		template<typename T>
		void add_subsystem_array()
		{
			const char* type_name = typeid(T).name();

			assert(m_subsystem_arrays.find(type_name) == m_subsystem_arrays.end() && "SubsystemManager::add_subsystem_array - Attempting to add subsystem array more than once");

			std::shared_ptr<SubsystemArray<T>> subsystem_array = std::make_shared<SubsystemArray<T>>();
			m_subsystem_arrays.emplace(type_name, subsystem_array);
		}

		template<typename T>
		SubsystemArray<T>& get_subsystem_array()
		{
			const char* type_name = typeid(T).name();

			assert(m_subsystem_arrays.find(type_name) != m_subsystem_arrays.end() && "SubsystemManager::get_subsystem_array - Attempting to get non valid subsystem array");

			return static_cast<SubsystemArray<T>>(m_subsystem_arrays.at(type_name));
		}
	};
}
