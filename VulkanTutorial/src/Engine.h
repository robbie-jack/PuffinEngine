#pragma once

#ifndef ENGINE_H
#define ENGINE_H

#include <ECS/ECS.h>
#include <SerializeScene.h>
#include <ProjectSettings.h>
#include "Engine/Subsystem.hpp"

#include <glfw/glfw3.h>

#include <vector>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <chrono>

namespace fs = std::filesystem;

namespace Puffin
{
	namespace Audio
	{
		class AudioSubsystem;
	}

	namespace UI
	{
		class UIManager;
	}
}

namespace Puffin::Core
{
	enum class PlayState
	{
		STARTED,		// Game has just started, gameplay systems need to be initialized
		PLAYING,		// Game is playing, all systems being updated
		JUST_STOPPED,	// Game has just been stopped, perform all system stop functions
		STOPPED,		// Game is stopped, no physics or game code is begin run, all data is in default state
		JUST_PAUSED,	// Game has just been paused
		PAUSED,			// Game is paused, systems not being updated,
		JUST_UNPAUSED	// Game has just been unpaused
	};

	class Engine : public std::enable_shared_from_this<Engine>
	{
	public:

		Engine() = default;
		~Engine() = default;

		void Init();
		bool Update();
		void Destroy();

		void Play();
		void Restart();
		void Exit();

		// Subsystem Methods
		template<typename SubsystemT>
		std::shared_ptr<SubsystemT> RegisterSubsystem()
		{
			const char* typeName = typeid(SubsystemT).name();

			assert(m_subsystemTypes.find(typeName) == m_subsystemTypes.end() && "Registering subsystem more than once");

			// Create subsystem pointer
			std::shared_ptr<SubsystemT> subsystem = std::make_shared<SubsystemT>();
			std::shared_ptr<Subsystem> subsystemBase = std::static_pointer_cast<Subsystem>(subsystem);
			subsystemBase->SetEngine(shared_from_this());

			// Cast subsystem to Subsystem parent and add to subsystems map
			m_subsystems.insert({ typeName, subsystemBase });

			return subsystem;
		}

		template<typename SubsystemT>
		std::shared_ptr<SubsystemT> GetSubsystem()
		{
			const char* typeName = typeid(SubsystemT).name();

			assert(m_subsystemTypes.find(typeName) != m_subsystemTypes.end() && "Subsystem used before registering.");

			return std::static_pointer_cast<SubsystemT>(m_subsystems[typeName]);
		}

		// Register System using Engine method because their update methods will not be called each frame otherwise
		template<typename SystemT>
		std::shared_ptr<SystemT> RegisterSystem()
		{
			std::shared_ptr<SystemT> system = nullptr;

			if (auto ecsWorld = GetSubsystem<ECS::World>())
			{
				system = ecsWorld->RegisterSystem<SystemT>();
				auto systemBase = std::static_pointer_cast<ECS::System>(system);

				m_systems.push_back(systemBase);
				m_systemUpdateVectors[systemBase->GetInfo().updateOrder].push_back(systemBase);
			}

			return nullptr;
		}

		PlayState GetPlayState() const { return playState; }
		inline std::shared_ptr<IO::SceneData> GetScene() { return m_sceneData; }

		inline IO::ProjectSettings& GetProjectSettings() { return settings; }

		GLFWwindow* GetWindow() const
		{
			return m_window;
		}

		std::shared_ptr<UI::UIManager> GetUIManager() const
		{
			return m_uiManager;
		}

		void SetTimeStep(const double timeStep)
		{
			m_timeStep = timeStep;
		}

	private:

		GLFWwindow* m_window = nullptr;
		GLFWmonitor* m_monitor = nullptr;

		std::shared_ptr<UI::UIManager> m_uiManager = nullptr;

		bool running = true;
		PlayState playState = PlayState::STOPPED;

		std::chrono::time_point<std::chrono::steady_clock> m_lastTime, m_currentTime;
		double m_accumulatedTime, m_timeStep, m_maxTimeStep;

		std::vector<std::shared_ptr<ECS::System>> m_systems; // Vector of system pointers
		std::map<Core::UpdateOrder, std::vector<std::shared_ptr<ECS::System>>> m_systemUpdateVectors; // Map from update order to system pointers

		IO::ProjectFile projectFile;

		IO::ProjectSettings settings;

		std::shared_ptr<IO::SceneData> m_sceneData = nullptr;

		// Subsystem Members
		std::unordered_map<const char*, std::shared_ptr<Core::Subsystem>> m_subsystems;

		void AddDefaultAssets();

		void DefaultScene(std::shared_ptr<ECS::World> world);
		void PhysicsScene(std::shared_ptr<ECS::World> world);

	};
}

#endif // !ENGINE_H