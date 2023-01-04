#pragma once

#include <ECS/ECS.h>
#include <SerializeScene.h>
#include <ProjectSettings.h>
#include "Engine/Subsystem.hpp"

#include <vector>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <map>
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
		std::shared_ptr<SubsystemT> RegisterSubsystem(uint8_t priority = 100)
		{
			const char* typeName = typeid(SubsystemT).name();

			assert(m_subsystems.find(typeName) == m_subsystems.end() && "Registering subsystem more than once");

			// Insert into priority map
			m_subsystemsPriority.insert(std::pair(priority, typeName));

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

			assert(m_subsystems.find(typeName) != m_subsystems.end() && "Subsystem used before registering.");

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
				m_systemUpdateVectors.insert(std::pair(systemBase->GetInfo().updateOrder, systemBase));

				m_systemExecutionTime[system->GetInfo().updateOrder][system->GetInfo().name] = 0.0;
			}

			return system;
		}

		/*
		 * shouldSerialize - Should this component be serialized to scene data
		 */
		template<typename CompT>
		void RegisterComponent(bool shouldSerialize = true)
		{
			if (auto ecsWorld = GetSubsystem<ECS::World>())
			{
				ecsWorld->RegisterComponent<CompT>();
			}

			if (m_sceneData != nullptr && shouldSerialize)
			{
				m_sceneData->RegisterComponent<CompT>();
			}
		}

		PlayState GetPlayState() const { return playState; }
		inline std::shared_ptr<IO::SceneData> GetScene() { return m_sceneData; }

		inline IO::ProjectSettings& GetProjectSettings() { return settings; }

		std::shared_ptr<UI::UIManager> GetUIManager() const
		{
			return m_uiManager;
		}

		const double& GetTimeStep() const
		{
			return m_timeStep;
		}

		void SetTimeStep(const double timeStep)
		{
			m_timeStep = timeStep;
		}

		const double& GetDeltaTime() const
		{
			return m_deltaTime;
		}

		const double& GetAccumulatedTime() const
		{
			return m_accumulatedTime;
		}

		const double& GetStageExecutionTime(const Core::UpdateOrder& updateOrder)
		{
			return m_stageExecutionTime[updateOrder];
		}

		const double& GetSystemExecutionTime(const Core::UpdateOrder& updateOrder, const std::string& systemName)
		{
			return m_systemExecutionTime[updateOrder][systemName];
		}

		const std::unordered_map<std::string, double>& GetSystemExecutionTimeForUpdateStage(const Core::UpdateOrder& updateOrder)
		{
			return m_systemExecutionTime[updateOrder];
		}

	private:

		std::shared_ptr<UI::UIManager> m_uiManager = nullptr;

		bool running = true;
		PlayState playState = PlayState::STOPPED;

		// Time Members
		std::chrono::time_point<std::chrono::steady_clock> m_lastTime, m_currentTime;
		double m_deltaTime = 0.0; // How long it took last frame to complete
		double m_accumulatedTime = 0.0; // Time passed since last physics tick
		double m_timeStep = 1 / 60.0; // How often deterministic code like physics should occur (defaults to 60 times a second)
		double m_maxTimeStep = 1 / 25.0; // Maximum amount of time each frame should take to complete

		// System Members
		std::vector<std::shared_ptr<ECS::System>> m_systems; // Vector of system pointers
		std::unordered_multimap<Core::UpdateOrder, std::shared_ptr<ECS::System>> m_systemUpdateVectors; // Map from update order to system pointers

		std::unordered_map<Core::UpdateOrder, double> m_stageExecutionTime; // Map of time it takes each stage of engine to execute (Physics, Rendering, Gameplay, etc...)
		std::unordered_map<Core::UpdateOrder, std::unordered_map<std::string, double>> m_systemExecutionTime; // Map of time it takes for each system to execute

		IO::ProjectFile projectFile;

		IO::ProjectSettings settings;

		std::shared_ptr<IO::SceneData> m_sceneData = nullptr;

		// Subsystem Members
		std::unordered_map<const char*, std::shared_ptr<Core::Subsystem>> m_subsystems;
		std::multimap<uint8_t, const char*> m_subsystemsPriority;

		void AddDefaultAssets();
		void ReimportDefaultAssets();

		void DefaultScene();
		void PhysicsScene();
		void ProceduralScene();

	};
}