#include "core/engine.h"

#include <thread>

#include "audio/audio_subsystem.h"
#include "core/engine_helpers.h"
#include "core/settings_manager.h"
#include "input/input_subsystem.h"
#include "scene/scene_serialization_subsystem.h"
#include "window/window_subsystem.h"
#include "subsystem/subsystem_manager.h"
#include "utility/benchmark.h"
#include "platform.h"
#include "scene/scene_info.h"
#include "rendering/render_subsystem.h"
#include "resource/resource_manager.h"
#include "subsystem/engine_subsystem.h"
#include "subsystem/gameplay_subsystem.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace puffin
{
	void AddDefaultEngineArguments(argparse::ArgumentParser& parser)
	{
		parser.add_argument("-p", "-project", "-project_path")
		      .help("Specify the path of the project file")
		      .required();

		parser.add_argument("-s", "-scene")
		      .help("Specify the scene file to load on launch")
		      .default_value("");

		parser.add_argument("--setup-default-scene-2d")
		      .help("Specify whether the engine default 2d scene should be initialized on launch")
		      .default_value(false)
		      .implicit_value(true);

		parser.add_argument("--setup-default-scene-3d")
			.help("Specify whether the engine default 3d scene should be initialized on launch")
			.default_value(false)
			.implicit_value(true);

		parser.add_argument("--setup-default-physics-scene-2d")
			  .help("Specify whether the engine default physics 2d scene should be initialized on launch")
			  .default_value(false)
			  .implicit_value(true);

		parser.add_argument("--setup-default-physics-scene-3d")
			.help("Specify whether the engine default physics 3d scene should be initialized on launch")
			.default_value(false)
			.implicit_value(true);

		parser.add_argument("--setup-default-settings")
		      .help("Specify whether to setup settings file with engine default settings")
		      .default_value(false)
		      .implicit_value(true);
	}
}

namespace puffin::core
{
	Engine::Engine()
	{
	}

	Engine::~Engine()
	{
		mEditor = nullptr;
		mApplication = nullptr;
		mPlatform = nullptr;
		mResourceManager = nullptr;
		mSubsystemManager = nullptr;
	}

	void Engine::Initialize(const argparse::ArgumentParser& parser)
	{
		// Pre-Initialization Setup
		mResourceManager = std::make_unique<ResourceManager>();
		mSubsystemManager = std::make_unique<SubsystemManager>(shared_from_this());
		utility::BenchmarkManager::Get();

		RegisterRequiredSubsystems(shared_from_this());

		if (mPlatform)
		{
			mPlatform->PreInitialize();
		}

		if (mApplication)
		{
			mApplication->PreInitialize();
		}

		mSubsystemManager->CreateAndPreInitializeEngineSubsystems();

		// Initialization
		fs::path projectPath = fs::path(parser.get<std::string>("-project_path")).make_preferred();

		// Load Project File
		LoadProject(projectPath, mProjectFile);

		mResourceManager->Initialize(mProjectFile, projectPath);

		// Load Project Settings
		mSetupEngineDefaultSettings = parser.get<bool>("--setup-default-settings");

		const bool setupDefaultScene2D = parser.get<bool>("--setup-default-scene-2d");
		const bool setupDefaultScene3D = parser.get<bool>("--setup-default-scene-3d");
		const bool setupDefaultPhysicsScene2D = parser.get<bool>("--setup-default-physics-scene-2d");
		const bool setupDefaultPhysicsScene3D = parser.get<bool>("--setup-default-physics-scene-3d");

		// Initialize engine subsystems
		if (mPlatform)
		{
			mPlatform->Initialize();
		}

		if (mApplication)
		{
			mApplication->Initialize();
		}

		std::vector<EngineSubsystem*> engineSubsystems;
		mSubsystemManager->GetEngineSubsystems(engineSubsystems);
		for (auto subsystem : engineSubsystems)
		{
			subsystem->Initialize();
		}

		// Post-Initialization Setup
		if (mPlatform)
		{
			mPlatform->PostInitialize();
		}

		if (mApplication)
		{
			mApplication->PostInitialize();
		}

		for (auto subsystem : engineSubsystems)
		{
			subsystem->PostInitialize();
		}

		// Scene Loading

		// Load default scene
		auto sceneString = parser.get<std::string>("-scene");
		if (!sceneString.empty())
		{
			mLoadSceneOnLaunch = true;
		}

		auto sceneSubsystem = GetSubsystem<scene::SceneSerializationSubsystem>();
		{
			scene::SceneInfo sceneInfo;

			if (setupDefaultScene2D)
			{
				sceneInfo.sceneType = scene::SceneType::Scene2D;

				auto sceneData = sceneSubsystem->CreateScene(mResourceManager->GetProjectPath() / sceneString, sceneInfo);

				SetupDefaultScene2D(shared_from_this());

				sceneData->UpdateData(shared_from_this());
				sceneData->Save();
			}
			else if (setupDefaultScene3D)
			{
				sceneInfo.sceneType = scene::SceneType::Scene3D;

				auto sceneData = sceneSubsystem->CreateScene(mResourceManager->GetProjectPath() / sceneString, sceneInfo);

				// Create Default Scene in code -- used when scene serialization is changed
				SetupDefaultScene3D(shared_from_this());

				sceneData->UpdateData(shared_from_this());
			}
			else if (setupDefaultPhysicsScene2D)
			{
				sceneInfo.sceneType = scene::SceneType::Scene2D;

				auto sceneData = sceneSubsystem->CreateScene(mResourceManager->GetProjectPath() /sceneString, sceneInfo);

				SetupDefaultPhysicsScene2D(shared_from_this());

				sceneData->UpdateData(shared_from_this());
				//sceneData->Save();
			}
			else if (setupDefaultPhysicsScene3D)
			{
				sceneInfo.sceneType = scene::SceneType::Scene3D;

				auto sceneData = sceneSubsystem->CreateScene(mResourceManager->GetProjectPath() / sceneString, sceneInfo);

				SetupDefaultPhysicsScene3D(shared_from_this());

				sceneData->UpdateData(shared_from_this());
			}
			else if (mLoadSceneOnLaunch)
			{
				sceneSubsystem->LoadFromFile(mResourceManager->GetProjectPath() / sceneString);
				sceneSubsystem->Setup();
			}
		}

		auto sceneData = sceneSubsystem->GetCurrentSceneData();
		if (!sceneData)
		{
			scene::SceneInfo sceneInfo;
			sceneInfo.sceneType = scene::SceneType::Scene2D;

			sceneData = sceneSubsystem->CreateScene(mResourceManager->GetProjectPath() / "new_scene.pscene", sceneInfo);
		}

		mCurrentSceneType = sceneSubsystem->GetCurrentSceneData()->GetSceneInfo().sceneType;

		for (auto subsystem : engineSubsystems)
		{
			subsystem->PostSceneLoad();
		}

		InitSettings();
		InitSignals();

		mLastTime = GetTime(); // Time Count Started
		mCurrentTime = mLastTime;

		mRunning = true;
		mPlayState = PlayState::Stopped;
	}

	bool Engine::Update()
	{
		// Run Game Loop;
		utility::BenchmarkManager* benchmarkManager = utility::BenchmarkManager::Get();

		// Process input
		{
			benchmarkManager->Begin("Input");

			auto inputSubsystem = mSubsystemManager->GetInputSubsystem();

			inputSubsystem->ProcessInput();

			benchmarkManager->End("Input");
		}

		// Wait for last presentation to complete and sample delta time
		{
			benchmarkManager->Begin("WaitForLastPresentationAndSample");

			auto renderSubsystem = mSubsystemManager->GetRenderSubsystem();

			double sampledTime = renderSubsystem->WaitForLastPresentationAndSampleTime();
			UpdateDeltaTime(sampledTime);

			benchmarkManager->End("WaitForLastPresentationAndSample");
		}

		const auto audioSubsystem = GetSubsystem<audio::AudioSubsystem>();

		// Execute engine updates
		{
			auto* engineUpdateBenchmark = benchmarkManager->Begin("EngineUpdate");

			std::vector<EngineSubsystem*> engineSubsystems;
			mSubsystemManager->GetEngineSubsystems(engineSubsystems);
			for (auto subsystem : engineSubsystems)
			{
				if (subsystem->ShouldUpdate())
				{
					engineUpdateBenchmark->Begin(subsystem->GetName());

					subsystem->Update(mDeltaTime);

					engineUpdateBenchmark->End(subsystem->GetName());
				}
			}

			if (mApplication && mApplication->ShouldEngineUpdate())
			{
				engineUpdateBenchmark->Begin(mApplication->GetName());
				
				mApplication->EngineUpdate(mDeltaTime);

				engineUpdateBenchmark->End(mApplication->GetName());
			}

			benchmarkManager->End("EngineUpdate");
		}

		// Call system start functions to prepare for gameplay
		if (mPlayState == PlayState::BeginPlay)
		{
			mSubsystemManager->CreateAndPreInitializeGameplaySubsystems();

			if (mApplication)
			{
				mApplication->BeginPlay();
			}

			std::vector<EngineSubsystem*> engineSubsystems;
			mSubsystemManager->GetEngineSubsystems(engineSubsystems);
			for (auto subsystem : engineSubsystems)
			{
				subsystem->BeginPlay();
			}

			std::vector<GameplaySubsystem*> gameplaySubsystems;
			mSubsystemManager->GetGameplaySubsystems(gameplaySubsystems);
			for (auto subsystem : gameplaySubsystems)
			{
				subsystem->BeginPlay();
			}

			mAccumulatedTime = 0.0;
			mPlayState = PlayState::Playing;
		}

		if (mPlayState == PlayState::JustPaused)
		{
			//audioSubsystem->pauseAllSounds();

			mPlayState = PlayState::Paused;
		}

		if (mPlayState == PlayState::JustUnpaused)
		{
			//audioSubsystem->playAllSounds();

			mPlayState = PlayState::Playing;
		}

		if (mPlayState == PlayState::Playing)
		{
			// Fixed Update
			{
				auto* fixedUpdateBenchmark = benchmarkManager->Begin("FixedUpdate");

				// Add onto accumulated time
				mAccumulatedTime += mDeltaTime;

				mAccumulatedTime = std::min(mAccumulatedTime, 1.0);

				while (mAccumulatedTime >= mTimeStepFixed)
				{
					mAccumulatedTime -= mTimeStepFixed;

					if (mApplication && mApplication->ShouldFixedUpdate())
					{
						fixedUpdateBenchmark->Begin(mApplication->GetName());
						
						mApplication->FixedUpdate(mTimeStepFixed);

						fixedUpdateBenchmark->End(mApplication->GetName());
					}

					std::vector<GameplaySubsystem*> gameplaySubsystems;
					mSubsystemManager->GetGameplaySubsystems(gameplaySubsystems);
					for (auto subsystem : gameplaySubsystems)
					{
						if (subsystem->ShouldFixedUpdate())
						{
							fixedUpdateBenchmark->Begin(subsystem->GetName());

							subsystem->FixedUpdate(mTimeStepFixed);

							fixedUpdateBenchmark->End(subsystem->GetName());
						}
					}
				}

				benchmarkManager->End("FixedUpdate");
			}

			// Update
			{
				auto* updateBenchmark = benchmarkManager->Begin("Update");

				if (mApplication && mApplication->ShouldUpdate())
				{
					updateBenchmark->Begin(mApplication->GetName());
					
					mApplication->Update(mDeltaTime);

					updateBenchmark->End(mApplication->GetName());
				}

				std::vector<GameplaySubsystem*> gameplaySubsystems;
				mSubsystemManager->GetGameplaySubsystems(gameplaySubsystems);
				for (auto subsystem : gameplaySubsystems)
				{
					if (subsystem->ShouldUpdate())
					{
						updateBenchmark->Begin(subsystem->GetName());

						subsystem->Update(mDeltaTime);

						updateBenchmark->End(subsystem->GetName());
					}
				}

				benchmarkManager->End("Update");
			}
		}

		// Render
		{
			benchmarkManager->Begin("Render");

			auto* renderSubsystem = mSubsystemManager->GetRenderSubsystem();

			renderSubsystem->Render(mDeltaTime);

			benchmarkManager->End("Render");
		}

		if (mPlayState == PlayState::EndPlay)
		{
			EndPlay();

			//audioSubsystem->stopAllSounds();
			benchmarkManager->Clear();

			mAccumulatedTime = 0.0;
			mPlayState = PlayState::Stopped;
		}

		if (const auto windowSubsystem = mSubsystemManager->GetWindowSubsystem(); windowSubsystem->ShouldPrimaryWindowClose())
		{
			mRunning = false;
		}

		return mRunning;
	}

	void Engine::Deinitialize()
	{
		// Cleanup any running gameplay subsystems
		if (mPlayState == PlayState::Playing)
		{
			EndPlay();
		}

		std::vector<EngineSubsystem*> engineSubsystems;
		mSubsystemManager->GetEngineSubsystems(engineSubsystems);
		for (auto subsystem : engineSubsystems)
		{
			subsystem->Deinitialize();
		}

		// Cleanup all engine subsystems
		mSubsystemManager->DestroyEngineSubsystems();

		if (mApplication)
		{
			mApplication->Deinitialize();
		}

		if (mPlatform)
		{
			mPlatform->Deinitialize();
		}

		utility::BenchmarkManager::Destroy();
	}

	void Engine::Play()
	{
		switch (mPlayState)
		{
		case PlayState::Stopped:

			mPlayState = PlayState::BeginPlay;
			break;

		case PlayState::Playing:

			mPlayState = PlayState::JustPaused;
			break;

		case PlayState::Paused:

			mPlayState = PlayState::JustUnpaused;
			break;

		default: ;
		}
	}

	void Engine::Restart()
	{
		if (mPlayState == PlayState::Playing || mPlayState == PlayState::Paused || mPlayState == PlayState::Stopped)
		{
			mPlayState = PlayState::EndPlay;
		}
	}

	void Engine::Exit()
	{
		mRunning = false;
	}

	scene::SceneType Engine::GetCurrentSceneType() const
	{
		return mCurrentSceneType;
	}

	void Engine::SetEditor(std::shared_ptr<editor::Editor> editor)
	{
		mEditor = editor;
	}

	std::shared_ptr<editor::Editor> Engine::GetEditor()
	{
		return mEditor;
	}

	bool Engine::IsEditorRunning() const
	{
		return mEditor != nullptr;
	}

	ResourceManager* Engine::GetResourceManager() const
	{
		return mResourceManager.get();
	}

	window::WindowSubsystem* Engine::GetWindowSubsystem() const
	{
		return mSubsystemManager->GetWindowSubsystem();
	}

	input::InputSubsystem* Engine::GetInputSubsystem() const
	{
		return mSubsystemManager->GetInputSubsystem();
	}

	rendering::RenderSubsystem* Engine::GetRenderSubsystem() const
	{
		return mSubsystemManager->GetRenderSubsystem();
	}

	void Engine::InitSettings()
	{
		auto settingsManager = GetSubsystem<core::SettingsManager>();

		mFramerateLimit = settingsManager->Get<int>("general", "framerate_limit").value_or(0);

		UpdatePhysicsTickRate(settingsManager->Get<uint16_t>("physics", "ticks_per_second").value_or(60));
	}

	void Engine::InitSignals()
	{
		const auto signalSubsystem = GetSubsystem<SignalSubsystem>();

		auto framerateLimitSignal = signalSubsystem->GetOrCreateSignal("general_framerate_limit");
		framerateLimitSignal->Connect(std::function([&]
			{
				auto settingsManager = GetSubsystem<core::SettingsManager>();

				mFramerateLimit = settingsManager->Get<int>("general", "framerate_limit").value_or(0);
			}));

		auto ticksPerSecondSignal = signalSubsystem->GetOrCreateSignal("physics_ticks_per_second");
		ticksPerSecondSignal->Connect(std::function([&]
			{
				auto settingsManager = GetSubsystem<core::SettingsManager>();

				UpdatePhysicsTickRate(settingsManager->Get<uint16_t>("physics", "ticks_per_second").value_or(60));
			}));
	}

	void Engine::EndPlay() const
	{
		// End play and cleanup gameplay subsystems
		std::vector<GameplaySubsystem*> gameplaySubsystems;
		mSubsystemManager->GetGameplaySubsystems(gameplaySubsystems);
		for (auto subsystem : gameplaySubsystems)
		{
			subsystem->EndPlay();
		}

		for (auto subsystem : gameplaySubsystems)
		{
			subsystem->Deinitialize();
		}

		mSubsystemManager->DestroyGameplaySubsystems();

		std::vector<EngineSubsystem*> engineSubsystems;
		mSubsystemManager->GetEngineSubsystems(engineSubsystems);
		for (auto subsystem : engineSubsystems)
		{
			subsystem->EndPlay();
		}

		if (mApplication)
		{
			mApplication->EndPlay();
		}
	}

	void Engine::UpdateDeltaTime(double sampledTime)
	{
		mCurrentTime = sampledTime;
		mDeltaTime = mCurrentTime - mLastTime;
		mLastTime = mCurrentTime;
	}

	void Engine::UpdatePhysicsTickRate(uint16_t ticksPerSecond)
	{
		mPhysicsTicksPerSecond = ticksPerSecond;
		mTimeStepFixed = 1.0 / mPhysicsTicksPerSecond;
	}
}
