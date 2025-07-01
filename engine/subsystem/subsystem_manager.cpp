#include "subsystem/subsystem_manager.h"

#include "core/engine.h"
#include "subsystem/engine_subsystem.h"
#include "subsystem/editor_subsystem.h"
#include "input/input_subsystem.h"
#include "physics/box2d/box2d_physics_subsystem.h"
#include "rendering/render_subsystem.h"
#include "window/window_subsystem.h"
#include "subsystem/gameplay_subsystem.h"
#include "subsystem/subsystem_reflection.h"

namespace puffin::core
{
	SubsystemManager::SubsystemManager(const std::shared_ptr<Engine>& engine) : m_engine(engine)
	{
	}

	SubsystemManager::~SubsystemManager()
	{
		DestroyGameplaySubsystems();
		DestroyGameplaySubsystems();

		m_engine = nullptr;
	}

	void SubsystemManager::CreateAndPreInitializeEngineSubsystems()
	{
		for (const auto& typeId : reflection::SubsystemRegistry::Get()->GetRegisteredTypesInOrder())
		{
			if (m_subsystems.find(typeId) != m_subsystems.end())
				continue;

			auto type = entt::resolve(typeId);

			if (!type.can_cast(entt::resolve<EngineSubsystem>()))
				continue;

			auto* subsystem = CreateSubsystem(typeId);
			subsystem->PreInitialize(this);
		}
	}

	void SubsystemManager::CreateAndPreInitializeGameplaySubsystems()
	{
		for (const auto& typeId : reflection::SubsystemRegistry::Get()->GetRegisteredTypesInOrder())
		{
			if (m_subsystems.find(typeId) != m_subsystems.end())
				continue;

			auto type = entt::resolve(typeId);

			if (!type.can_cast(entt::resolve<GameplaySubsystem>()))
				continue;

			auto* subsystem = CreateSubsystem(typeId);
			subsystem->PreInitialize(this);
		}
	}

	void SubsystemManager::DestroyEngineSubsystems()
	{
		if (m_engineSubsystem.empty())
			return;

		for (auto typeId : m_engineSubsystem)
		{
			DestroySubsystem(typeId);
		}

		m_engineSubsystem.clear();
		m_editorSubsystem.clear();
	}

	void SubsystemManager::DestroyGameplaySubsystems()
	{
		if (m_gameplaySubsystems.empty())
			return;

		for (auto typeId : m_gameplaySubsystems)
		{
			DestroySubsystem(typeId);
		}

		m_gameplaySubsystems.clear();
	}

	void SubsystemManager::GetEngineSubsystems(std::vector<EngineSubsystem*>& subsystems)
	{
		subsystems.reserve(m_engineSubsystem.size());

		for (auto typeId : m_engineSubsystem)
		{
			subsystems.push_back(dynamic_cast<EngineSubsystem*>(m_subsystems.at(typeId)));
		}
	}

	void SubsystemManager::GetGameplaySubsystems(std::vector<GameplaySubsystem*>& subsystems)
	{
		subsystems.reserve(m_gameplaySubsystems.size());

		for (auto typeId : m_gameplaySubsystems)
		{
			subsystems.push_back(dynamic_cast<GameplaySubsystem*>(m_subsystems.at(typeId)));
		}
	}

	window::WindowSubsystem* SubsystemManager::GetWindowSubsystem() const
	{
		assert(m_windowSubsystem != 0 && "SubsystemManager::GetWindowSubsystem() - Attempting to get window subsystem while it is invalid");

		return dynamic_cast<window::WindowSubsystem*>(m_subsystems.at(m_windowSubsystem));
	}

	input::InputSubsystem* SubsystemManager::GetInputSubsystem() const
	{
		assert(m_inputSubsystem != 0 && "SubsystemManager::GetInputSubsystem() - Attempting to get input subsystem while it is invalid");

		return dynamic_cast<input::InputSubsystem*>(m_subsystems.at(m_inputSubsystem));
	}

	rendering::RenderSubsystem* SubsystemManager::GetRenderSubsystem() const
	{
		assert(m_renderSubsystem != 0 && "SubsystemManager::GetRenderSubsystem() - Attempting to get render subsystem while it is invalid");

		return dynamic_cast<rendering::RenderSubsystem*>(m_subsystems.at(m_renderSubsystem));
	}

	Subsystem* SubsystemManager::CreateSubsystem(entt::id_type typeId)
	{
		// Return if subsystem of this type is already created
		if (m_subsystems.find(typeId) != m_subsystems.end())
			return m_subsystems.at(typeId);

		auto type = entt::resolve(typeId);

		if (!type.can_cast(entt::resolve<Subsystem>()))
			return nullptr;

		if (type.can_cast(entt::resolve<EditorSubsystem>()) && !m_engine->IsEditorRunning())
			return nullptr;

		auto createSubsystemFunc = type.func(entt::hs("CreateSubsystem"));
		auto* subsystem = createSubsystemFunc.invoke({}, m_engine).cast<Subsystem*>();
		m_subsystems.emplace(typeId, subsystem);

		if (type.can_cast(entt::resolve<EngineSubsystem>()))
		{
			if (type.can_cast(entt::resolve<window::WindowSubsystem>()))
			{
				assert(m_windowSubsystem == 0 && "SubsystemManager::CreateSubsystem - Attempting to create a second window subsystem");

				m_windowSubsystem = typeId;
			}

			if (type.can_cast(entt::resolve<input::InputSubsystem>()))
			{
				assert(m_inputSubsystem == 0 && "SubsystemManager::CreateSubsystem - Attempting to create a second input subsystem");

				m_inputSubsystem = typeId;
			}

			if (type.can_cast(entt::resolve<rendering::RenderSubsystem>()))
			{
				assert(m_renderSubsystem == 0 && "SubsystemManager::CreateSubsystem - Attempting to create a second render subsystem");

				m_renderSubsystem = typeId;
			}

			if (type.can_cast(entt::resolve<EditorSubsystem>()))
			{
				m_editorSubsystem.push_back(typeId);
			}

			m_engineSubsystem.push_back(typeId);
		}

		if (type.can_cast(entt::resolve<GameplaySubsystem>()))
		{
			m_gameplaySubsystems.push_back(typeId);
		}

		return m_subsystems.at(typeId);
	}

	void SubsystemManager::DestroySubsystem(entt::id_type typeId)
	{
		if (m_subsystems.find(typeId) == m_subsystems.end())
			return;

		delete m_subsystems.at(typeId);
		m_subsystems.erase(typeId);

		if (m_windowSubsystem == typeId)
			m_windowSubsystem = 0;

		if (m_inputSubsystem == typeId)
			m_inputSubsystem = 0;

		if (m_renderSubsystem == typeId)
			m_renderSubsystem = 0;
	}
}
