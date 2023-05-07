#pragma once

#include "Application.h"
#include "ProjectSettings.h"
#include "SerializeScene.h"
#include "Subsystem.h"
#include "System.h"

#include <glfw/glfw3.h>

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace puffin
{
	namespace audio
	{
		class AudioSubsystem;
	}

	namespace ui
	{
		class UISubsystem;
	}
}

namespace puffin::core
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

	const std::vector<std::pair<ExecutionStage, const std::string>> gExecutionStageOrder =
	{
		{ ExecutionStage::Idle, "Idle" },
		{ ExecutionStage::FixedUpdate, "FixedUpdate" },
		{ ExecutionStage::Update, "Update" },
		{ ExecutionStage::Render, "Render" },
	};

	enum class PlayState
	{
		Started,		// Game has just started, gameplay systems need to be initialized
		Playing,		// Game is playing, all systems being updated
		JustStopped,	// Game has just been stopped, perform all system stop functions
		Stopped,		// Game is stopped, no physics or game code is begin run, all data is in default state
		JustPaused,	// Game has just been paused
		Paused,			// Game is paused, systems not being updated,
		JustUnpaused	// Game has just been unpaused
	};

	// Handler class for executing functions in engine
	class EngineCallbackHandler
	{
	public:

		EngineCallbackHandler(const std::function<void()>& callback, const std::string& name, const uint8_t& priority) :
			mCallback(callback), mName(name), mPriority(priority) {}

		void execute() const
		{
			mCallback();
		}

		const std::string& name() const
		{
			return mName;
		}

		bool operator<(const EngineCallbackHandler& other) const
		{
			return mPriority < other.mPriority;
		}

	private:

		std::function<void()> mCallback;
		std::string mName;
		uint8_t mPriority;

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
			assert(mApplication == nullptr && "Registering multiple applications");

			mApplication = std::static_pointer_cast<Application>(std::make_shared<Application>());
		}

		// Subsystem Methods
		template<typename SubsystemT>
		std::shared_ptr<SubsystemT> registerSubsystem()
		{
			const char* typeName = typeid(SubsystemT).name();

			assert(mSubsystems.find(typeName) == mSubsystems.end() && "Registering subsystem more than once");

			// Create subsystem pointer
			auto subsystem = std::make_shared<SubsystemT>();
			auto subsystemBase = std::static_pointer_cast<Subsystem>(subsystem);
			subsystemBase->setEngine(shared_from_this());

			subsystemBase->setupCallbacks();

			// Cast subsystem to Subsystem parent and add to subsystems map
			mSubsystems.insert({ typeName, subsystemBase });

			return subsystem;
		}

		template<typename SubsystemT>
		std::shared_ptr<SubsystemT> getSubsystem()
		{
			const char* typeName = typeid(SubsystemT).name();

			assert(mSubsystems.find(typeName) != mSubsystems.end() && "Subsystem used before registering.");

			return std::static_pointer_cast<SubsystemT>(mSubsystems[typeName]);
		}

		// Register System using Engine method because their update methods will not be called each frame otherwise
		template<typename SystemT>
		std::shared_ptr<SystemT> registerSystem()
		{
			std::shared_ptr<SystemT> system = std::make_shared<SystemT>();

			auto systemBase = std::static_pointer_cast<core::System>(system);

			systemBase->setEngine(shared_from_this());
			systemBase->setupCallbacks();

			mSystems.push_back(systemBase);

			return system;
		}

		// Register a callback function to be executed during engine runtime
		void registerCallback(ExecutionStage executionStage, const std::function<void()>& callback, const std::string& name = "", const int& priority = 100)
		{
			// Add function handler to vector
			mRegisteredCallbacks[executionStage].emplace_back(callback, name, priority);

			// Sort vector by priority
			std::sort(mRegisteredCallbacks[executionStage].begin(), mRegisteredCallbacks[executionStage].end());
		}

		PlayState playState() const { return mPlayState; }
		inline std::shared_ptr<io::SceneData> sceneData() { return mSceneData; }

		inline io::ProjectSettings& settings() { return mSettings; }

		bool shouldRenderEditorUI() const { return mShouldRenderEditorUI; }

		const double& timeStepFixed() const { return mTimeStepFixed; }

		void setTimeStepFixed(const double timeStepFixed) { mTimeStepFixed = timeStepFixed; }

		const double& deltaTime() const { return mDeltaTime; }

		const double& accumulatedTime() const { return mAccumulatedTime; }

		double getStageExecutionTimeLastFrame(const core::ExecutionStage& updateOrder)
		{
			return mStageExecutionTimeLastFrame[updateOrder];
		}

		const std::unordered_map<std::string, double>& getCallbackExecutionTimeForUpdateStageLastFrame(const core::ExecutionStage& updateOrder)
		{
			return mCallbackExecutionTimeLastFrame[updateOrder];
		}

	private:

		bool mRunning = true;
		bool mShouldLimitFrame = true; // Whether framerate should be capped at m_frameRateMax
		bool mShouldTrackExecutionTime = true; // Should track time to execute callback/stages

		bool mShouldRenderEditorUI = false; // Whether editor UI should be rendered

		PlayState mPlayState = PlayState::Stopped;

		// Framerate Members
		uint16_t mFrameRateMax = 0; // Limit on how fast game runs
		uint16_t mPhysicsTicksPerFrame = 60; // How many times physics code should run per frame

		// Time Members
		double mLastTime = 0.0;
		double mCurrentTime = 0.0;
		double mDeltaTime = 0.0; // How long it took last frame to complete
		double mAccumulatedTime = 0.0; // Time passed since last physics tick
		double mTimeStepFixed = 1.0 / mPhysicsTicksPerFrame; // How often deterministic code like physics should occur (defaults to 60 times a second)
		double mTimeStepLimit = 1 / 25.0; // Maximum amount of time each frame should take to complete

		std::shared_ptr<Application> mApplication = nullptr;

		// System/Subsystem Members
		std::vector<std::shared_ptr<core::System>> mSystems; // Vector of system pointers
		std::unordered_map<const char*, std::shared_ptr<core::Subsystem>> mSubsystems;
		std::unordered_map<core::ExecutionStage, std::vector<EngineCallbackHandler>> mRegisteredCallbacks; // Map of callback functions registered for execution

		std::unordered_map<core::ExecutionStage, double> mStageExecutionTime; // Map of time it takes each stage of engine to execute (Physics, Rendering, Gameplay, etc...)
		std::unordered_map<core::ExecutionStage, std::unordered_map<std::string, double>> mCallbackExecutionTime; // Map of time it takes for each system to execute

		std::unordered_map<core::ExecutionStage, double> mStageExecutionTimeLastFrame;
		std::unordered_map<core::ExecutionStage, std::unordered_map<std::string, double>> mCallbackExecutionTimeLastFrame;

		io::ProjectFile mProjectFile;

		io::ProjectSettings mSettings;

		std::shared_ptr<io::SceneData> mSceneData = nullptr;

		// Execute callbacks for this execution stage
		void executeCallbacks(const core::ExecutionStage& executionStage, const bool shouldTrackExecutionTime = false)
		{
			double startTime = 0.0, endTime = 0.0;
			double stageStartTime = 0.0;

			if (mShouldTrackExecutionTime && shouldTrackExecutionTime)
			{
				stageStartTime = glfwGetTime();
			}

			for (const auto& callback : mRegisteredCallbacks[executionStage])
			{
				if (mShouldTrackExecutionTime && shouldTrackExecutionTime)
				{
					startTime = glfwGetTime();
				}

				callback.execute();

				if (mShouldTrackExecutionTime && shouldTrackExecutionTime)
				{
					endTime = glfwGetTime();

					if (mCallbackExecutionTime[executionStage].count(callback.name()) == 0)
					{
						mCallbackExecutionTime[executionStage][callback.name()] = 0.0;
					}

					mCallbackExecutionTime[executionStage][callback.name()] += endTime - startTime;
				}
			}

			if (mShouldTrackExecutionTime && shouldTrackExecutionTime)
			{
				const double stageEndTime = glfwGetTime();

				if (mStageExecutionTime.count(executionStage) == 0)
				{
					mStageExecutionTime[executionStage] = 0.0;
				}

				mStageExecutionTime[executionStage] += (stageEndTime - stageStartTime);
			}
		}

		void updateExecutionTime()
		{
			if (mShouldTrackExecutionTime)
			{
				mStageExecutionTimeLastFrame.clear();
				mCallbackExecutionTimeLastFrame.clear();

				mStageExecutionTimeLastFrame = mStageExecutionTime;
				mCallbackExecutionTimeLastFrame = mCallbackExecutionTime;

				mStageExecutionTime.clear();
				mCallbackExecutionTime.clear();
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