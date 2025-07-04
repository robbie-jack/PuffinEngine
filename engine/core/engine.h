#pragma once

#include <memory>

#include "argparse/argparse.hpp"
#include "core/application.h"
#include "subsystem/subsystem_manager.h"
#include "project_settings.h"
#include "types/scene_type.h"

namespace fs = std::filesystem;

namespace puffin
{
    void AddDefaultEngineArguments(argparse::ArgumentParser& parser);

	class ResourceManager;

	namespace editor
	{
		class Editor;
	}

	namespace window
	{
		class WindowSubsystem;
	}

	namespace input
	{
		class InputSubsystem;
	}

	namespace rendering
	{
		class RenderSubsystem;
	}
}

namespace puffin::core
{
	class Platform;

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

		void Initialize(const argparse::ArgumentParser& parser);
		bool Update();
		void Deinitialize();

		void Play();
		void Restart();
		void Exit();

		PlayState GetPlayState() const { return mPlayState; }
		scene::SceneType GetCurrentSceneType() const;

		bool GetSetupEngineDefaultSettings() const { return mSetupEngineDefaultSettings; }

		uint16_t GetFramerateLimit() const { return mFramerateLimit; }

		const double& GetLastTime() const { return mLastTime; }
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

		template<typename PlatT>
		void RegisterPlatform()
		{
			assert(mPlatform == nullptr && "Registering multile platforms");

			mPlatform = std::static_pointer_cast<Platform>(std::make_shared<PlatT>(shared_from_this()));
		}

		/*template<typename T>
		void RegisterSubsystem() const
		{
			mSubsystemManager->RegisterSubsystem<T>();
		}*/

		template<typename T>
		T* GetSubsystem() const
		{
			return mSubsystemManager->GetSubsystem<T>();
		}

		void SetEditor(std::shared_ptr<editor::Editor> editor);
		std::shared_ptr<editor::Editor> GetEditor();
		bool IsEditorRunning() const;

		ResourceManager* GetResourceManager() const;

		window::WindowSubsystem* GetWindowSubsystem() const;
		input::InputSubsystem* GetInputSubsystem() const;
		rendering::RenderSubsystem* GetRenderSubsystem() const;

	private:

		void InitSettings();
		void InitSignals();

		void EndPlay() const;

		void UpdateDeltaTime(double sampledTime);
		void UpdatePhysicsTickRate(uint16_t ticksPerSecond);

		bool mRunning = true;
		bool mLoadSceneOnLaunch = false;
		bool mSetupEngineDefaultSettings = false;

		PlayState mPlayState = PlayState::Stopped;
		scene::SceneType mCurrentSceneType = scene::SceneType::Invalid;

		// Framerate Members
		uint16_t mFramerateLimit = 0; // Limit on how fast game runs
		uint16_t mPhysicsTicksPerSecond = 60; // How many times physics code should run per frame

		// Time Members
		double mLastTime = 0.0; // Time since engine launch at start of last frame
		double mCurrentTime = 0.0; // Time since engine launch at start of current frame
		double mDeltaTime = 0.0; // How long it took last frame to complete
		double mAccumulatedTime = 0.0; // Time passed since last physics tick
		double mTimeStepFixed = 1.0 / mPhysicsTicksPerSecond; // How often deterministic code like physics should occur (defaults to 60 times a second)

		std::shared_ptr<editor::Editor> mEditor = nullptr;
		std::shared_ptr<Application> mApplication = nullptr;
		std::shared_ptr<Platform> mPlatform = nullptr;
		std::unique_ptr<ResourceManager> mResourceManager = nullptr;
		std::unique_ptr<SubsystemManager> mSubsystemManager = nullptr;

		io::ProjectFile mProjectFile;

	};
}
