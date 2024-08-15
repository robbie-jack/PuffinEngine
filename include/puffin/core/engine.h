#pragma once

#include "puffin/core/application.h"
#include "puffin/projectsettings.h"
#include "puffin/core/subsystem.h"
#include "argparse/argparse.hpp"

#include <GLFW/glfw3.h>

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

#include "subsystemmanager.h"

namespace fs = std::filesystem;

namespace puffin
{
    void AddDefaultEngineArguments(argparse::ArgumentParser& parser);
}

namespace puffin::core
{
	enum class PlayState
	{
		BeginPlay,		// Game has just started, gameplay systems need to be initialized
		Playing,		// Game is playing, all systems being updated
		EndPlay,	// Game has just been stopped, perform all system stop functions
		Stopped,		// Game is stopped, no physics or game code is begin run, all data is in default state
		JustPaused,		// Game has just been paused
		Paused,			// Game is paused, systems not being updated,
		JustUnpaused	// Game has just been unpaused
	};

	class Engine : public std::enable_shared_from_this<Engine>
	{
	public:

		Engine();
		~Engine();

		void Setup();
		void Initialize(const argparse::ArgumentParser& pS);
		bool Update();
		void Deinitialize();

		void Play();
		void Restart();
		void Exit();

		template<typename AppT>
		void RegisterApp()
		{
			assert(mApplication == nullptr && "Registering multiple applications");

			mApplication = std::static_pointer_cast<Application>(std::make_shared<AppT>(shared_from_this()));
		}

		PlayState GetPlayState() const { return mPlayState; }

		bool GetSetupEngineDefaultSettings() const { return mSetupEngineDefaultSettings; }
		bool GetShouldRenderEditorUI() const { return mShouldRenderEditorUI; }

		const double& GetTimeStepFixed() const { return mTimeStepFixed; }
		const double& GetDeltaTime() const { return mDeltaTime; }
		const double& GetAccumulatedTime() const { return mAccumulatedTime; }

		template<typename T>
		void RegisterSubsystem() const
		{
			mSubsystemManager->RegisterSubsystem<T>();
		}

		template<typename T>
		T* GetSubsystem() const
		{
			return mSubsystemManager->GetSubsystem<T>();
		}

	private:

		bool mRunning = true;
		bool mLoadSceneOnLaunch = false;
		bool mSetupEngineDefaultScene = false;
		bool mSetupEngineDefaultSettings = false;
		bool mShouldLimitFramerate = true; // Whether framerate should be capped at m_frameRateMax
		bool mShouldRenderEditorUI = true; // Whether editor UI should be rendered

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
		std::unique_ptr<SubsystemManager> mSubsystemManager = nullptr;

		io::ProjectFile mProjectFile;

		void RegisterRequiredSubsystems() const;

		void AddDefaultAssets();
		void ReimportDefaultAssets();
		void LoadAndResaveAssets();

		void InitDefaultScene();
		void InitPhysicsScene3D();
		void InitProceduralScene();

		void UpdateDeltaTime(double sampledTime);

	};
}
