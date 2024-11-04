#pragma once

#include "puffin/core/application.h"
#include "puffin/projectsettings.h"
#include "argparse/argparse.hpp"

#include <GLFW/glfw3.h>

#include <memory>

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

	static constexpr uint32_t gEngineVersionMajor = 0;
	static constexpr uint32_t gEngineVersionMinor = 1;
	static constexpr uint32_t gEngineVersionPatch = 0;

	struct EngineVersion
	{
		uint32_t major, minor, patch;
	};

	class Engine : public std::enable_shared_from_this<Engine>
	{
	public:

		Engine();
		~Engine();

		void Setup();
		void Initialize(const argparse::ArgumentParser& parser);
		bool Update();
		void Deinitialize();

		void Play();
		void Restart();
		void Exit();

		PlayState GetPlayState() const { return mPlayState; }

		bool GetSetupEngineDefaultSettings() const { return mSetupEngineDefaultSettings; }
		bool GetShouldRenderEditorUI() const { return mEditorUIEnabled; }

		const double& GetTimeStepFixed() const { return mTimeStepFixed; }
		const double& GetDeltaTime() const { return mDeltaTime; }
		const double& GetAccumulatedTime() const { return mAccumulatedTime; }

		static EngineVersion GetEngineVersion()
		{
			return { gEngineVersionMajor, gEngineVersionMinor, gEngineVersionPatch };
		}

		static void GetEngineVersionString(std::string& version)
		{
			version.clear();
			version += std::to_string(gEngineVersionMajor) + ".";
			version += std::to_string(gEngineVersionMinor) + ".";
			version += std::to_string(gEngineVersionPatch);
		}

		template<typename AppT>
		void RegisterApp(const std::string& name)
		{
			assert(mApplication == nullptr && "Registering multiple applications");

			mApplication = std::static_pointer_cast<Application>(std::make_shared<AppT>(name, shared_from_this()));
		}

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

		void InitSettings();
		void InitSignals();
		void UpdateDeltaTime(double sampledTime);
		void UpdatePhysicsTickRate(uint16_t ticksPerSecond);
		void Idle();

		bool mRunning = true;
		bool mLoadSceneOnLaunch = false;
		bool mSetupEngineDefaultScene = false;
		bool mSetupEngineDefaultPhysics2DScene = false;
		bool mSetupEngineDefaultSettings = false;
		bool mFramerateLimitEnable = true; // Whether framerate should be capped at m_frameRateMax
		bool mEditorUIEnabled = true; // Whether editor UI should be rendered

		PlayState mPlayState = PlayState::Stopped;

		// Framerate Members
		uint16_t mFramerateLimit = 0; // Limit on how fast game runs
		uint16_t mPhysicsTicksPerSecond = 60; // How many times physics code should run per frame

		// Time Members
		double mLastTime = 0.0;
		double mCurrentTime = 0.0;
		double mDeltaTime = 0.0; // How long it took last frame to complete
		double mAccumulatedTime = 0.0; // Time passed since last physics tick
		double mTimeStepFixed = 1.0 / mPhysicsTicksPerSecond; // How often deterministic code like physics should occur (defaults to 60 times a second)
		double mTimeStepLimit = 1 / 30.0; // Maximum amount of time each frame should take to complete

		std::shared_ptr<Application> mApplication = nullptr;
		std::unique_ptr<SubsystemManager> mSubsystemManager = nullptr;

		io::ProjectFile mProjectFile;

	};
}
