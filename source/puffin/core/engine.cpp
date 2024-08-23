#include "puffin/core/engine.h"

#include <chrono>
#include <thread>

#include "puffin/assets/assetimporters.h"
#include "puffin/assets/assetregistry.h"
#include "puffin/assets/staticmeshasset.h"
#include "puffin/audio/audiosubsystem.h"
#include "puffin/core/enginehelpers.h"
#include "puffin/input/inputsubsystem.h"
#include "puffin/scene/scenegraphsubsystem.h"
#include "puffin/scene/sceneserializationsubsystem.h"
#include "puffin/window/windowsubsystem.h"
#include "puffin/core/subsystemmanager.h"
#include "puffin/utility/performancebenchmarksubsystem.h"

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

		parser.add_argument("--setup-engine-default-scene")
		      .help("Specify whether the engine default scene should be initialized on launch")
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
		mApplication = nullptr;
		mSubsystemManager = nullptr;
	}

	void Engine::Setup()
	{
		mSubsystemManager = std::make_unique<SubsystemManager>(shared_from_this());

		RegisterRequiredSubsystems(shared_from_this());
		RegisterComponentTypes();

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
		mSetupEngineDefaultScene = parser.get<bool>("--setup-engine-default-scene");

		// Load/Initialize Assets
		assets::AssetRegistry::Get()->LoadAssetCache();

		// Initialize engine subsystems
		mSubsystemManager->CreateAndInitializeEngineSubsystems();

		if (mApplication)
		{
			mApplication->Initialize();
		}

		// Register components to scene subsystem
		RegisterComponents(shared_from_this());

		// Load default scene
		auto sceneString = parser.get<std::string>("-scene");
		if (!sceneString.empty())
		{
			mLoadSceneOnLaunch = true;
		}

		auto sceneSubsystem = GetSubsystem<io::SceneSerializationSubsystem>();
		{
			auto sceneData = sceneSubsystem->CreateScene(
				assets::AssetRegistry::Get()->GetContentRoot() / sceneString);

			if (mSetupEngineDefaultScene)
			{
				auto entt_subsystem = GetSubsystem<ecs::EnTTSubsystem>();
				auto scene_graph = GetSubsystem<scene::SceneGraphSubsystem>();

				// Create Default Scene in code -- used when scene serialization is changed
				SetupDefaultScene(shared_from_this());

				sceneData->UpdateData(entt_subsystem, scene_graph);
				sceneData->Save();
			}
			else if (mLoadSceneOnLaunch)
			{
				sceneData->Load();
			}
		}

		sceneSubsystem->Setup();

		mLastTime = glfwGetTime(); // Time Count Started
		mCurrentTime = mLastTime;

		mRunning = true;
		mPlayState = PlayState::Stopped;
	}

	bool Engine::Update()
	{
		// Run Game Loop;

		// Process input
		{
			auto benchmarkSubsystem = GetSubsystem<utility::PerformanceBenchmarkSubsystem>();
			benchmarkSubsystem->StartBenchmark("Input");

			auto inputSubsystem = mSubsystemManager->GetInputSubsystem();
			inputSubsystem->ProcessInput();

			benchmarkSubsystem->EndBenchmark("Input");
		}

		// Wait for last presentation to complete and sample delta time
		{
			auto benchmarkSubsystem = GetSubsystem<utility::PerformanceBenchmarkSubsystem>();
			benchmarkSubsystem->StartBenchmark("Sample Time");

			auto renderSubsystem = mSubsystemManager->GetRenderSubsystem();

			double sampledTime = renderSubsystem->WaitForLastPresentationAndSampleTime();
			UpdateDeltaTime(sampledTime);

			benchmarkSubsystem->EndBenchmark("Sample Time");
		}

		// If frame rate cap is enabled, idle app until needed
		{
			auto benchmarkSubsystem = GetSubsystem<utility::PerformanceBenchmarkSubsystem>();
			benchmarkSubsystem->StartBenchmark("Idle Time");

			Idle();

			benchmarkSubsystem->EndBenchmark("Idle Time");
		}

		// Make sure delta time never exceeds 1/30th of a second
		if (mDeltaTime > mTimeStepLimit)
		{
			mDeltaTime = mTimeStepLimit;
		}

		const auto audioSubsystem = GetSubsystem<audio::AudioSubsystem>();

		// Execute engine updates
		{
			auto benchmarkSubsystem = GetSubsystem<utility::PerformanceBenchmarkSubsystem>();
			benchmarkSubsystem->StartBenchmark("Engine Update");

			for (auto subsystem : mSubsystemManager->GetSubsystems())
			{
				if (subsystem->ShouldUpdate())
				{
					benchmarkSubsystem->StartBenchmarkCategory(subsystem->GetName(), "Engine Update");

					subsystem->Update(mDeltaTime);

					benchmarkSubsystem->EndBenchmarkCategory(subsystem->GetName(), "Engine Update");
				}
			}

			if (mApplication && mApplication->ShouldEngineUpdate())
			{
				mApplication->EngineUpdate(mDeltaTime);
			}

			benchmarkSubsystem->EndBenchmark("Engine Update");
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

			for (auto subsystem : mSubsystemManager->GetSubsystems())
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
				auto benchmarkSubsystem = GetSubsystem<utility::PerformanceBenchmarkSubsystem>();
				benchmarkSubsystem->StartBenchmark("Fixed Update");

				// Add onto accumulated time
				mAccumulatedTime += mDeltaTime;

				while (mAccumulatedTime >= mTimeStepFixed)
				{
					mAccumulatedTime -= mTimeStepFixed;

					if (mApplication && mApplication->ShouldFixedUpdate())
					{
						mApplication->FixedUpdate(mTimeStepFixed);
					}

					for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
					{
						if (subsystem->ShouldFixedUpdate())
						{
							benchmarkSubsystem->StartBenchmarkCategory(subsystem->GetName(), "Fixed Update");

							subsystem->FixedUpdate(mTimeStepFixed);

							benchmarkSubsystem->EndBenchmarkCategory(subsystem->GetName(), "Fixed Update");
						}
					}
				}

				benchmarkSubsystem->EndBenchmark("Fixed Update");
			}

			// Update
			{
				auto benchmarkSubsystem = GetSubsystem<utility::PerformanceBenchmarkSubsystem>();
				benchmarkSubsystem->StartBenchmark("Update");

				if (mApplication && mApplication->ShouldUpdate())
				{
					mApplication->Update(mDeltaTime);
				}

				for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
				{
					if (subsystem->ShouldUpdate())
					{
						benchmarkSubsystem->StartBenchmarkCategory(subsystem->GetName(), "Update");

						subsystem->Update(mDeltaTime);

						benchmarkSubsystem->EndBenchmarkCategory(subsystem->GetName(), "Update");
					}
				}

				benchmarkSubsystem->EndBenchmark("Update");
			}
		}

		// Render
		{
			auto benchmarkSubsystem = GetSubsystem<utility::PerformanceBenchmarkSubsystem>();
			benchmarkSubsystem->StartBenchmark("Render");

			auto renderSubsystem = mSubsystemManager->GetRenderSubsystem();

			renderSubsystem->Render(mDeltaTime);

			benchmarkSubsystem->EndBenchmark("Render");
		}

		if (mPlayState == PlayState::EndPlay)
		{
			// End play and cleanup gameplay subsystems
			for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
			{
				subsystem->EndPlay();
			}

			if (mApplication)
			{
				mApplication->EndPlay();
			}

			for (auto subsystem : mSubsystemManager->GetSubsystems())
			{
				subsystem->EndPlay();
			}

			mSubsystemManager->DestroyGameplaySubsystems();

			//audioSubsystem->stopAllSounds();

			mAccumulatedTime = 0.0;
			mPlayState = PlayState::Stopped;
		}

		if (const auto windowSubsystem = GetSubsystem<window::WindowSubsystem>(); windowSubsystem->
			GetShouldPrimaryWindowClose())
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
			if (mApplication)
			{
				mApplication->EndPlay();
			}

			for (auto subsystem : mSubsystemManager->GetGameplaySubsystems())
			{
				subsystem->EndPlay();
			}

			for (auto subsystem : mSubsystemManager->GetSubsystems())
			{
				subsystem->EndPlay();
			}

			mSubsystemManager->DestroyGameplaySubsystems();
		}

		if (mApplication)
		{
			mApplication->Deinitialize();
		}

		// Cleanup all engine subsystems
		mSubsystemManager->DestroyEngineSubsystems();

		// Clear Asset Registry
		assets::AssetRegistry::Clear();

		mSubsystemManager = nullptr;
	}

	void Engine::Play()
	{
		if (mPlayState == PlayState::Stopped)
		{
			mPlayState = PlayState::BeginPlay;
		}
		else if (mPlayState == PlayState::Playing)
		{
			mPlayState = PlayState::JustPaused;
		}
		else if (mPlayState == PlayState::Paused)
		{
			mPlayState = PlayState::JustUnpaused;
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

	void Engine::UpdateDeltaTime(double sampledTime)
	{
		mLastTime = mCurrentTime;
		mCurrentTime = sampledTime;
		mDeltaTime = mCurrentTime - mLastTime;
	}

	void Engine::Idle()
	{
		if (mFrameRateMax > 0)
		{
			const double deltaTimeMax = 1.0 / mFrameRateMax;
			double idleStartTime = 0.0;

			while (mDeltaTime < deltaTimeMax)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(0));

				mCurrentTime = glfwGetTime();
				mDeltaTime = mCurrentTime - mLastTime;
			}
		}
	}
}
