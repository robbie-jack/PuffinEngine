#include "puffin/core/engine.h"

#include <chrono>
#include <thread>

#include "puffin/rendering/rendersubsystem.h"
#include "puffin/assets/assetimporters.h"
#include "puffin/assets/assetregistry.h"
#include "puffin/assets/staticmeshasset.h"
#include "puffin/audio/audiosubsystem.h"
#include "puffin/core/enginehelpers.h"
#include "puffin/core/settingsmanager.h"
#include "puffin/input/inputsubsystem.h"
#include "puffin/scene/scenegraphsubsystem.h"
#include "puffin/scene/sceneserializationsubsystem.h"
#include "puffin/window/windowsubsystem.h"
#include "puffin/core/subsystemmanager.h"
#include "puffin/core/timer.h"
#include "puffin/rendering/camerasubsystem.h"
#include "puffin/utility/benchmark.h"
#include "puffin/platform/platform.h"

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
		mSubsystemManager = nullptr;
	}

	void Engine::Setup()
	{
		mSubsystemManager = std::make_unique<SubsystemManager>(shared_from_this());
		utility::BenchmarkManager::Get();

		RegisterRequiredSubsystems(shared_from_this());

		if (mPlatform)
		{
			mPlatform->RegisterTypes();
		}

		if (mApplication)
		{
			mApplication->RegisterTypes();
		}
	}

	void Engine::Initialize(const argparse::ArgumentParser& parser)
	{
		fs::path projectPath = fs::path(parser.get<std::string>("-project_path")).make_preferred();

		// Load Project File
		LoadProject(projectPath, mProjectFile);

		// Setup asset registry
		RegisterAssetTypes(mProjectFile, projectPath);

		// Load Project Settings
		mSetupEngineDefaultSettings = parser.get<bool>("--setup-default-settings");

		const bool setupDefaultScene2D = parser.get<bool>("--setup-default-scene-2d");
		const bool setupDefaultScene3D = parser.get<bool>("--setup-default-scene-3d");
		const bool setupDefaultPhysicsScene2D = parser.get<bool>("--setup-default-physics-scene-2d");
		const bool setupDefaultPhysicsScene3D = parser.get<bool>("--setup-default-physics-scene-3d");

		// Load/Initialize Assets
		assets::AssetRegistry::Get()->LoadAssetCache();

		// Initialize engine subsystems
		mSubsystemManager->CreateAndInitializeEngineSubsystems();

		if (mApplication)
		{
			mApplication->Initialize();
		}

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

				auto sceneData = sceneSubsystem->CreateScene(assets::AssetRegistry::Get()->GetContentRoot() / sceneString, sceneInfo);

				SetupDefaultScene2D(shared_from_this());

				sceneData->UpdateData(shared_from_this());
				sceneData->Save();
			}
			else if (setupDefaultScene3D)
			{
				sceneInfo.sceneType = scene::SceneType::Scene3D;

				auto sceneData = sceneSubsystem->CreateScene(assets::AssetRegistry::Get()->GetContentRoot() / sceneString, sceneInfo);

				// Create Default Scene in code -- used when scene serialization is changed
				SetupDefaultScene3D(shared_from_this());

				sceneData->UpdateData(shared_from_this());
			}
			else if (setupDefaultPhysicsScene2D)
			{
				sceneInfo.sceneType = scene::SceneType::Scene2D;

				auto sceneData = sceneSubsystem->CreateScene(assets::AssetRegistry::Get()->GetContentRoot() / sceneString, sceneInfo);

				SetupDefaultPhysicsScene2D(shared_from_this());

				sceneData->UpdateData(shared_from_this());
				sceneData->Save();
			}
			else if (setupDefaultPhysicsScene3D)
			{
				sceneInfo.sceneType = scene::SceneType::Scene3D;

				auto sceneData = sceneSubsystem->CreateScene(assets::AssetRegistry::Get()->GetContentRoot() / sceneString, sceneInfo);

				SetupDefaultPhysicsScene3D(shared_from_this());

				sceneData->UpdateData(shared_from_this());
			}
			else if (mLoadSceneOnLaunch)
			{
				sceneSubsystem->LoadFromFile(assets::AssetRegistry::Get()->GetContentRoot() / sceneString);
				sceneSubsystem->Setup();
			}
		}

		mCurrentSceneType = sceneSubsystem->GetCurrentSceneData()->GetSceneInfo().sceneType;

		for (auto subsystem : mSubsystemManager->GetEngineSubsystems())
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

			if (inputSubsystem->IsActionPressed("editor_play_pause"))
				Play();

			if (inputSubsystem->IsActionPressed("editor_restart"))
				Restart();

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

			for (auto subsystem : mSubsystemManager->GetEngineSubsystems())
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

		/*const auto inputSubsystem = getSystem<input::InputSubsystem>();
		if (inputSubsystem->justPressed("Play"))
		{
			play();
		}

		if (inputSubsystem->justPressed("Restart"))
		{
			restart();
		}*/

		// Call system start functions to prepare for gameplay
		if (mPlayState == PlayState::BeginPlay)
		{
			mSubsystemManager->CreateAndInitializeGameplaySubsystems();

			for (auto subsystem : mSubsystemManager->GetEngineSubsystems())
			{
				subsystem->BeginPlay();
			}

			if (mApplication)
			{
				mApplication->BeginPlay();
			}

			for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
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

					for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
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

				for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
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

			auto renderSubsystem = mSubsystemManager->GetRenderSubsystem();

			renderSubsystem->Render(mDeltaTime);

			benchmarkManager->End("Render");
		}

		if (mPlayState == PlayState::EndPlay)
		{
			// End play and cleanup gameplay subsystems
			for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
			{
				subsystem->EndPlay();
			}

			mSubsystemManager->DestroyGameplaySubsystems();

			if (mApplication)
			{
				mApplication->EndPlay();
			}

			for (auto subsystem : mSubsystemManager->GetEngineSubsystems())
			{
				subsystem->EndPlay();
			}

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
			for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
			{
				subsystem->EndPlay();
			}

			mSubsystemManager->DestroyGameplaySubsystems();

			if (mApplication)
			{
				mApplication->EndPlay();
			}

			for (auto subsystem : mSubsystemManager->GetEngineSubsystems())
			{
				subsystem->EndPlay();
			}
		}

		if (mApplication)
		{
			mApplication->Deinitialize();
		}

		// Cleanup all engine subsystems
		mSubsystemManager->DestroyEngineSubsystems();

		// Clear Asset Registry
		assets::AssetRegistry::Clear();

		utility::BenchmarkManager::Destroy();
		mSubsystemManager = nullptr;
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
