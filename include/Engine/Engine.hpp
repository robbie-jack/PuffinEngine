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
		idle,				// Only used for calculating idle time when frame rate is limited, do not use with callback
		init,				// Occurs once on engine launch, use for one off system initialization
		setup,				// Occurs on engine launch and whenever gameplay is stopped
		start,				// Occurs whenever gameplay is started
		subsystemUpdate,	// Occurs every frame, regardless if game is currently playing/paused
		fixedUpdate,		// Updates happen at a fixed rate, and can occur multiple times in a single frame - Useful for physics or code which should be deterministic
		update,				// Update once a frame - Useful for non-determinstic gameplay code
		render,				// Update once a frame - Useful for code which relates to the rendering pipeline
		stop,				// Occurs when game play is stopped, use for resetting any gameplay data
		cleanup				// Occurs when engine exits, use for cleaning up all data
	};

	const std::vector<std::pair<ExecutionStage, const std::string>> gExecutionStageOrder =
	{
		{ ExecutionStage::idle, "Idle" },
		{ ExecutionStage::fixedUpdate, "FixedUpdate" },
		{ ExecutionStage::update, "Update" },
		{ ExecutionStage::render, "Render" },
	};

	enum class PlayState
	{
		started,		// Game has just started, gameplay systems need to be initialized
		playing,		// Game is playing, all systems being updated
		justStopped,	// Game has just been stopped, perform all system stop functions
		stopped,		// Game is stopped, no physics or game code is begin run, all data is in default state
		justPaused,	// Game has just been paused
		paused,			// Game is paused, systems not being updated,
		justUnpaused	// Game has just been unpaused
	};

	// Handler class for executing functions in engine
	class EngineCallbackHandler
	{
	public:

		EngineCallbackHandler(const std::function<void()>& callback, const std::string& name, const uint8_t& priority) :
			callback_(callback), name_(name), priority_(priority) {}

		void execute() const
		{
			callback_();
		}

		const std::string& name() const
		{
			return name_;
		}

		bool operator<(const EngineCallbackHandler& other) const
		{
			return priority_ < other.priority_;
		}

	private:

		std::function<void()> callback_;
		std::string name_;
		uint8_t priority_;

	};

	class Engine : public std::enable_shared_from_this<Engine>
	{
	public:

		Engine() = default;
		~Engine() = default;

		void init();
		bool update();
		void destroy();

		void play();
		void restart();
		void exit();

		template<typename AppT>
		void registerApp()
		{
			assert(m_application == nullptr && "Registering multiple applications");

			application_ = std::static_pointer_cast<Application>(std::make_shared<Application>());
		}

		// Subsystem Methods
		template<typename SubsystemT>
		std::shared_ptr<SubsystemT> registerSubsystem()
		{
			const char* typeName = typeid(SubsystemT).name();

			assert(subsystems_.find(typeName) == subsystems_.end() && "Registering subsystem more than once");

			// Create subsystem pointer
			auto subsystem = std::make_shared<SubsystemT>();
			auto subsystemBase = std::static_pointer_cast<Subsystem>(subsystem);
			subsystemBase->SetEngine(shared_from_this());

			subsystemBase->SetupCallbacks();

			// Cast subsystem to Subsystem parent and add to subsystems map
			subsystems_.insert({ typeName, subsystemBase });

			return subsystem;
		}

		template<typename SubsystemT>
		std::shared_ptr<SubsystemT> getSubsystem()
		{
			const char* typeName = typeid(SubsystemT).name();

			assert(subsystems_.find(typeName) != subsystems_.end() && "Subsystem used before registering.");

			return std::static_pointer_cast<SubsystemT>(subsystems_[typeName]);
		}

		// Register System using Engine method because their update methods will not be called each frame otherwise
		template<typename SystemT>
		std::shared_ptr<SystemT> registerSystem()
		{
			std::shared_ptr<SystemT> system = nullptr;

			if (auto ecsWorld = getSubsystem<ECS::World>())
			{
				system = ecsWorld->registerSystem<SystemT>();
				auto systemBase = std::static_pointer_cast<ECS::System>(system);

				systemBase->SetWorld(ecsWorld);
				systemBase->SetEngine(shared_from_this());
				systemBase->SetupCallbacks();

				systems_.push_back(systemBase);
			}

			return system;
		}

		// Register a callback function to be executed during engine runtime
		void registerCallback(ExecutionStage executionStage, const std::function<void()>& callback, const std::string& name = "", const int& priority = 100)
		{
			// Add function handler to vector
			registeredCallbacks_[executionStage].emplace_back(callback, name, priority);

			// Sort vector by priority
			std::sort(registeredCallbacks_[executionStage].begin(), registeredCallbacks_[executionStage].end());
		}

		/*
		 * shouldSerialize - Should this component be serialized to scene data
		 */
		template<typename CompT>
		void registerComponent(bool shouldSerialize = true)
		{
			if (auto ecsWorld = getSubsystem<ECS::World>())
			{
				ecsWorld->RegisterComponent<CompT>();
			}

			if (sceneData_ != nullptr && shouldSerialize)
			{
				sceneData_->RegisterComponent<CompT>();
			}
		}

		PlayState playState() const { return playState_; }
		inline std::shared_ptr<IO::SceneData> sceneData() { return sceneData_; }

		inline IO::ProjectSettings& settings() { return settings_; }

		std::shared_ptr<UI::UIManager> uiManager() const
		{
			return uiManager_;
		}

		bool shouldRenderEditorUi() const { return shouldRenderEditorUi_; }

		const double& timeStepFixed() const { return timeStepFixed_; }

		void setTimeStepFixed(const double timeStepFixed) { timeStepFixed_ = timeStepFixed; }

		const double& deltaTime() const { return deltaTime_; }

		const double& accumulatedTime() const { return accumulatedTime_; }

		double getStageExecutionTimeLastFrame(const Core::ExecutionStage& updateOrder)
		{
			return stageExecutionTimeLastFrame_[updateOrder];
		}

		const std::unordered_map<std::string, double>& getCallbackExecutionTimeForUpdateStageLastFrame(const Core::ExecutionStage& updateOrder)
		{
			return callbackExecutionTimeLastFrame_[updateOrder];
		}

	private:

		std::shared_ptr<UI::UIManager> uiManager_ = nullptr;

		bool running_ = true;
		bool shouldLimitFrame_ = true; // Whether framerate should be capped at m_frameRateMax
		bool shouldRenderEditorUi_ = true; // Whether editor UI should be rendered
		bool shouldTrackExecutionTime_ = true; // Should track time to execute callback/stages
		PlayState playState_ = PlayState::stopped;

		// Framerate Members
		uint16_t frameRateMax_ = 0; // Limit on how fast game runs
		uint16_t physicsTicksPerFrame_ = 60; // How many times physics code should run per frame

		// Time Members
		double lastTime_ = 0.0;
		double currentTime_ = 0.0;
		double deltaTime_ = 0.0; // How long it took last frame to complete
		double accumulatedTime_ = 0.0; // Time passed since last physics tick
		double timeStepFixed_ = 1.0 / physicsTicksPerFrame_; // How often deterministic code like physics should occur (defaults to 60 times a second)
		double timeStepLimit_ = 1 / 25.0; // Maximum amount of time each frame should take to complete

		std::shared_ptr<Application> application_ = nullptr;

		// System/Subsystem Members
		std::vector<std::shared_ptr<ECS::System>> systems_; // Vector of system pointers
		std::unordered_map<const char*, std::shared_ptr<Core::Subsystem>> subsystems_;
		std::unordered_map<Core::ExecutionStage, std::vector<EngineCallbackHandler>> registeredCallbacks_; // Map of callback functions registered for execution

		std::unordered_map<Core::ExecutionStage, double> stageExecutionTime_; // Map of time it takes each stage of engine to execute (Physics, Rendering, Gameplay, etc...)
		std::unordered_map<Core::ExecutionStage, std::unordered_map<std::string, double>> callbackExecutionTime_; // Map of time it takes for each system to execute

		std::unordered_map<Core::ExecutionStage, double> stageExecutionTimeLastFrame_;
		std::unordered_map<Core::ExecutionStage, std::unordered_map<std::string, double>> callbackExecutionTimeLastFrame_;

		IO::ProjectFile projectFile_;

		IO::ProjectSettings settings_;

		std::shared_ptr<IO::SceneData> sceneData_ = nullptr;

		// Execute callbacks for this execution stage
		void executeCallbacks(const Core::ExecutionStage& executionStage, bool shouldTrackExecutionTime = false)
		{
			double startTime = 0.0, endTime = 0.0;
			double stageStartTime = 0.0, stageEndTime = 0.0;

			if (shouldTrackExecutionTime_ && shouldTrackExecutionTime)
			{
				stageStartTime = glfwGetTime();
			}

			for (const auto& callback : registeredCallbacks_[executionStage])
			{
				if (shouldTrackExecutionTime_ && shouldTrackExecutionTime)
				{
					startTime = glfwGetTime();
				}

				callback.execute();

				if (shouldTrackExecutionTime_ && shouldTrackExecutionTime)
				{
					endTime = glfwGetTime();

					if (callbackExecutionTime_[executionStage].count(callback.name()) == 0)
					{
						callbackExecutionTime_[executionStage][callback.name()] = 0.0;
					}

					callbackExecutionTime_[executionStage][callback.name()] += endTime - startTime;
				}
			}

			if (shouldTrackExecutionTime_ && shouldTrackExecutionTime)
			{
				stageEndTime = glfwGetTime();

				if (stageExecutionTime_.count(executionStage) == 0)
				{
					stageExecutionTime_[executionStage] = 0.0;
				}

				stageExecutionTime_[executionStage] += (stageEndTime - stageStartTime);
			}
		}

		void updateExecutionTime()
		{
			if (shouldTrackExecutionTime_)
			{
				stageExecutionTimeLastFrame_.clear();
				callbackExecutionTimeLastFrame_.clear();

				stageExecutionTimeLastFrame_ = stageExecutionTime_;
				callbackExecutionTimeLastFrame_ = callbackExecutionTime_;

				stageExecutionTime_.clear();
				callbackExecutionTime_.clear();
			}
		}

		void addDefaultAssets();
		void reimportDefaultAssets();
		void loadAndResaveAssets();

		void defaultScene();
		void physicsScene();
		void proceduralScene();

	};
}