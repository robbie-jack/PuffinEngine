#pragma once

#include <ECS/ECS.h>
#include <SerializeScene.h>
#include <ProjectSettings.h>
#include "Engine/Subsystem.hpp"
#include "Engine/Application.hpp"
#include <glfw/glfw3.h>

#include <vector>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <map>

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
	// Various stages when methods can be executed during engine runtime
	enum class ExecutionStage
	{
		Idle,				// Only used for calculating idle time when frame rate is limited, do not use with callback
		Init,				// Occurs once on engine launch, use for one off system initialization
		Setup,				// Occurs on engine launch and whenever gameplay is stopped
		Start,				// Occurs whenever gameplay is started
		SubsystemUpdate,	// Occurs every frame, regardless if game is currently playing/paused
		FixedUpdate,		// Updates happen at a fixed rate, and can occur multiple times in a single frame - Useful for physics or code which should be deterministic
		Update,				// Update once a frame - Useful for non-determinstic gameplay code
		Render,				// Update once a frame - Useful for code which relates to the rendering pipeline
		Stop,				// Occurs when game play is stopped, use for resetting any gameplay data
		Cleanup				// Occurs when engine exits, use for cleaning up all data
	};

	const std::vector<std::pair<ExecutionStage, const std::string>> G_EXECUTION_STAGE_ORDER =
	{
		{ ExecutionStage::Idle, "Idle" },
		{ ExecutionStage::FixedUpdate, "FixedUpdate" },
		{ ExecutionStage::Update, "Update" },
		{ ExecutionStage::Render, "Render" },
	};

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

	// Handler class for executing functions in engine
	class EngineCallbackHandler
	{
	public:

		EngineCallbackHandler(const std::function<void()>& callback, const std::string& name, const uint8_t& priority) :
			m_callback(callback), m_name(name), m_priority(priority) {}

		void Execute() const
		{
			m_callback();
		}

		const std::string& GetName() const
		{
			return m_name;
		}

		bool operator<(const EngineCallbackHandler& other) const
		{
			return m_priority < other.m_priority;
		}

	private:

		std::function<void()> m_callback;
		std::string m_name;
		uint8_t m_priority;

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

		template<typename AppT>
		void RegisterApp()
		{
			assert(m_application == nullptr && "Registering multiple applications");

			m_application = std::static_pointer_cast<Application>(std::make_shared<Application>());
		}

		// Subsystem Methods
		template<typename SubsystemT>
		std::shared_ptr<SubsystemT> RegisterSubsystem()
		{
			const char* typeName = typeid(SubsystemT).name();

			assert(m_subsystems.find(typeName) == m_subsystems.end() && "Registering subsystem more than once");

			// Create subsystem pointer
			auto subsystem = std::make_shared<SubsystemT>();
			auto subsystemBase = std::static_pointer_cast<Subsystem>(subsystem);
			subsystemBase->SetEngine(shared_from_this());

			subsystemBase->SetupCallbacks();

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

				systemBase->SetWorld(ecsWorld);
				systemBase->SetEngine(shared_from_this());
				systemBase->SetupCallbacks();

				m_systems.push_back(systemBase);
			}

			return system;
		}

		// Register a callback function to be executed during engine runtime
		void RegisterCallback(ExecutionStage executionStage, const std::function<void()>& callback, const std::string& name = "", const int& priority = 100)
		{
			// Add function handler to vector
			m_registeredCallbacks[executionStage].emplace_back(callback, name, priority);

			// Sort vector by priority
			std::sort(m_registeredCallbacks[executionStage].begin(), m_registeredCallbacks[executionStage].end());
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

		PlayState GetPlayState() const { return m_playState; }
		inline std::shared_ptr<IO::SceneData> GetScene() { return m_sceneData; }

		inline IO::ProjectSettings& GetProjectSettings() { return settings; }

		std::shared_ptr<UI::UIManager> GetUIManager() const
		{
			return m_uiManager;
		}

		bool ShouldRenderEditorUI() const { return m_shouldRenderEditorUI; }

		const double& GetTimeStep() const { return m_timeStepFixed; }

		void SetTimeStep(const double timeStep) { m_timeStepFixed = timeStep; }

		const double& GetDeltaTime() const { return m_deltaTime; }

		const double& GetAccumulatedTime() const { return m_accumulatedTime; }

		double GetStageExecutionTimeLastFrame(const Core::ExecutionStage& updateOrder)
		{
			double executionTime = 0.0;

			for (const auto& stageExecutionTime : m_stageExecutionTimeLastFrame[updateOrder])
			{
				executionTime += stageExecutionTime;
			}

			return executionTime;
		}

		const std::vector<std::pair<std::string, double>>& GetCallbackExecutionTimeForUpdateStageLastFrame(const Core::ExecutionStage& updateOrder)
		{
			return m_callbackExecutionTimeLastFrame[updateOrder];
		}

	private:

		std::shared_ptr<UI::UIManager> m_uiManager = nullptr;

		bool running = true;
		bool m_shouldLimitFrameRate = true; // Whether framerate should be capped at m_frameRateMax
		bool m_shouldRenderEditorUI = true; // Whether editor UI should be rendered
		bool m_shouldTrackExecutionTime = true; // Should track time to execute callback/stages
		PlayState m_playState = PlayState::STOPPED;

		// Framerate Members
		uint16_t m_frameRateMax = 0; // Limit on how fast game runs
		uint16_t m_physicsTicksPerFrame = 60; // How many times physics code should run per frame

		// Time Members
		double m_lastTime, m_currentTime;
		double m_deltaTime = 0.0; // How long it took last frame to complete
		double m_accumulatedTime = 0.0; // Time passed since last physics tick
		double m_timeStepFixed = 1.0 / m_physicsTicksPerFrame; // How often deterministic code like physics should occur (defaults to 60 times a second)
		double m_timeStepLimit = 1 / 25.0; // Maximum amount of time each frame should take to complete

		std::shared_ptr<Application> m_application = nullptr;

		// System/Subsystem Members
		std::vector<std::shared_ptr<ECS::System>> m_systems; // Vector of system pointers
		std::unordered_map<const char*, std::shared_ptr<Core::Subsystem>> m_subsystems;
		std::unordered_map<Core::ExecutionStage, std::vector<EngineCallbackHandler>> m_registeredCallbacks; // Map of callback functions registered for execution

		std::unordered_map<Core::ExecutionStage, std::vector<double>> m_stageExecutionTime; // Map of time it takes each stage of engine to execute (Physics, Rendering, Gameplay, etc...)
		std::unordered_map<Core::ExecutionStage, std::vector<std::pair<std::string, double>>> m_callbackExecutionTime; // Map of time it takes for each system to execute

		std::unordered_map<Core::ExecutionStage, std::vector<double>> m_stageExecutionTimeLastFrame;
		std::unordered_map<Core::ExecutionStage, std::vector<std::pair<std::string, double>>> m_callbackExecutionTimeLastFrame;

		IO::ProjectFile projectFile;

		IO::ProjectSettings settings;

		std::shared_ptr<IO::SceneData> m_sceneData = nullptr;

		// Execute callbacks for this execution stage
		void ExecuteCallbacks(const Core::ExecutionStage& executionStage, bool shouldTrackExecutionTime = false)
		{
			double startTime = 0.0, endTime = 0.0;
			double stageStartTime = 0.0, stageEndTime = 0.0;

			if (m_shouldTrackExecutionTime && shouldTrackExecutionTime)
			{
				stageStartTime = glfwGetTime();
			}

			for (const auto& callback : m_registeredCallbacks[executionStage])
			{
				if (m_shouldTrackExecutionTime && shouldTrackExecutionTime)
				{
					startTime = glfwGetTime();
				}

				callback.Execute();

				if (m_shouldTrackExecutionTime && shouldTrackExecutionTime)
				{
					endTime = glfwGetTime();

					m_callbackExecutionTime[executionStage].emplace_back(callback.GetName(), endTime - startTime);
				}
			}

			if (m_shouldTrackExecutionTime && shouldTrackExecutionTime)
			{
				stageEndTime = glfwGetTime();

				m_stageExecutionTime[executionStage].emplace_back(stageEndTime - stageStartTime);
			}
		}

		void UpdateExecutionTime()
		{
			if (m_shouldTrackExecutionTime)
			{
				m_stageExecutionTimeLastFrame.clear();
				m_callbackExecutionTimeLastFrame.clear();

				m_stageExecutionTimeLastFrame = m_stageExecutionTime;
				m_callbackExecutionTimeLastFrame = m_callbackExecutionTime;

				m_stageExecutionTime.clear();
				m_callbackExecutionTime.clear();
			}
		}

		void AddDefaultAssets();
		void ReimportDefaultAssets();
		void LoadAndResaveAssets();

		void DefaultScene();
		void PhysicsScene();
		void ProceduralScene();

	};
}