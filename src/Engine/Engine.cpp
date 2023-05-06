#include "Engine/Engine.h"

#include "ECS/ECS.h"
#include "ECS/Entity.h"

#include "Rendering/BGFX/BGFXRenderSystem.h"
#include "Rendering/Vulkan/VKRenderSystem.h"
#include "Physics/Box2D/Box2DPhysicsSystem.h"
#include "Physics/Onager2D/OnagerPhysicsSystem2D.h"
#include "Scripting/AngelScriptSystem.h"
#include "Procedural/ProceduralMeshGenSystem.h"

#include "Components/TransformComponent.h"
#include "Components/Scripting/AngelScriptComponent.h"
#include "Components/Procedural/ProceduralMeshComponent.h"

#include "Window/WindowSubsystem.h"
#include "Input/InputSubsystem.h"
#include "Engine/SignalSubsystem.h"
#include "Engine/EnkiTSSubsystem.h"
#include "ECS/EnTTSubsystem.h"

#include "SerializeScene.h"
#include "UI/Editor/UIManager.h"

#include "Audio/AudioSubsystem.h"

#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"
#include "Assets/TextureAsset.h"
#include "Assets/SoundAsset.h"

#include <chrono>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace puffin::core
{
	void Engine::init()
	{
		// Subsystems

		auto windowSubsystem = registerSubsystem<Window::WindowSubsystem>();
		auto signalSubsystem = registerSubsystem<SignalSubsystem>();
		auto enkitsSubsystem = registerSubsystem<EnkiTSSubsystem>();
		auto inputSubsystem = registerSubsystem<input::InputSubsystem>();
		auto audioSubsystem = registerSubsystem<audio::AudioSubsystem>();
		auto ecsWorld = registerSubsystem<ECS::World>();
		auto enttSubsystem = registerSubsystem<ECS::EnTTSubsystem>();

		mUiManager = std::make_shared<UI::UIManager>(shared_from_this());

		// Load Project File
		const auto projectPath = fs::path(R"(C:\Projects\PuffinProject\Puffin.pproject)");
		fs::path projectDirPath = projectPath;
		projectDirPath.remove_filename();

		LoadProject(projectPath, mProjectFile);

		// Load Default Scene (if set)
		fs::path defaultScenePath = projectDirPath.parent_path() / "content" / mProjectFile.defaultScenePath;
		mSceneData = std::make_shared<io::SceneData>(ecsWorld, defaultScenePath);

		// Register Components to ECS World and Scene Data Class
		mSceneData->RegisterComponent<SceneObjectComponent>();
		mSceneData->RegisterComponent<TransformComponent>();

		mSceneData->RegisterComponent<rendering::MeshComponent>();
		mSceneData->RegisterComponent<rendering::LightComponent>();
		mSceneData->RegisterComponent<rendering::ShadowCasterComponent>();
		mSceneData->RegisterComponent<rendering::CameraComponent>();

		mSceneData->RegisterComponent<physics::RigidbodyComponent2D>();
		mSceneData->RegisterComponent<physics::BoxComponent2D>();
		mSceneData->RegisterComponent<physics::CircleComponent2D>();

		mSceneData->RegisterComponent<scripting::AngelScriptComponent>();

		mSceneData->RegisterComponent<rendering::ProceduralMeshComponent>();
		mSceneData->RegisterComponent<procedural::PlaneComponent>();
		mSceneData->RegisterComponent<procedural::TerrainComponent>();
		mSceneData->RegisterComponent<procedural::IcoSphereComponent>();

		// Systems
		//registerSystem<Rendering::BGFX::BGFXRenderSystem>();
		registerSystem<rendering::VKRenderSystem>();
		registerSystem<physics::OnagerPhysicsSystem2D>();
		//registerSystem<Physics::Box2DPhysicsSystem>();
		registerSystem<scripting::AngelScriptSystem>();
		registerSystem<procedural::ProceduralMeshGenSystem>();

		// Register Assets
		assets::AssetRegistry::get()->registerAssetType<assets::StaticMeshAsset>();
		assets::AssetRegistry::get()->registerAssetType<assets::TextureAsset>();
		assets::AssetRegistry::get()->registerAssetType<assets::SoundAsset>();

		// Load Asset Cache
		assets::AssetRegistry::get()->setProjectName(mProjectFile.name);
		assets::AssetRegistry::get()->setProjectRoot(projectDirPath);

		// Load Project Settings
		io::LoadSettings(projectDirPath.parent_path() / "Settings.json", mSettings);

		// Load/Initialize Assets
		//AddDefaultAssets();
		assets::AssetRegistry::get()->loadAssetCache();
		//ReimportDefaultAssets();

		// Create Default Scene in code -- used when scene serialization is changed
		//defaultScene();
		physicsScene();
		//proceduralScene();

		// Load Scene -- normal behaviour
		//m_sceneData->LoadAndInit();
		//m_sceneData->Save();

		mRunning = true;
		mPlayState = PlayState::Stopped;

		// Initialize Systems
		{
			executeCallbacks(ExecutionStage::Init);
			executeCallbacks(ExecutionStage::Setup);
		}

		mLastTime = glfwGetTime(); // Time Count Started
		mCurrentTime = mLastTime;
	}

	bool Engine::update()
	{
		// Run Game Loop;
		mLastTime = mCurrentTime;
		mCurrentTime = glfwGetTime();
		mDeltaTime = mCurrentTime - mLastTime;

		updateExecutionTime();

		if (mFrameRateMax > 0)
		{
			const double deltaTimeMax = 1.0 / mFrameRateMax;
			double idleStartTime = 0.0;

			if (mShouldTrackExecutionTime)
			{
				// Sleep until next frame should start
				idleStartTime = glfwGetTime();
			}

			while (mDeltaTime < deltaTimeMax)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(0));

				mCurrentTime = glfwGetTime();
				mDeltaTime = mCurrentTime - mLastTime;
			}

			if (mShouldTrackExecutionTime)
			{
				double idleEndTime = glfwGetTime();

				mStageExecutionTime[ExecutionStage::Idle] = idleEndTime - idleStartTime;
			}
		}

		// Make sure delta time never exceeds 1/30th of a second
		if (mDeltaTime > mTimeStepLimit)
		{
			mDeltaTime = mTimeStepLimit;
		}

		auto ecsWorld = getSubsystem<ECS::World>();
		auto audioSubsystem = getSubsystem<audio::AudioSubsystem>();

		// Update all Subsystems
		{
			executeCallbacks(ExecutionStage::SubsystemUpdate, true);
		}

		auto inputSubsystem = getSubsystem<input::InputSubsystem>();
		if (inputSubsystem->getAction("Play").state == input::KeyState::Pressed)
		{
			play();
		}

		if (inputSubsystem->getAction("Restart").state == input::KeyState::Pressed)
		{
			restart();
		}

		// Call system start functions to prepare for gameplay
		if (mPlayState == PlayState::Started)
		{
			executeCallbacks(ExecutionStage::Start);

			// Get Snapshot of current scene data
			//m_sceneData->UpdateData();

			mAccumulatedTime = 0.0;
			mPlayState = PlayState::Playing;
		}

		if (mPlayState == PlayState::JustPaused)
		{
			audioSubsystem->pauseAllSounds();

			mPlayState = PlayState::Paused;
		}

		if (mPlayState == PlayState::JustUnpaused)
		{
			audioSubsystem->playAllSounds();

			mPlayState = PlayState::Playing;
		}

		if (mPlayState == PlayState::Playing)
		{
			// Fixed Update
			{
				// Add onto accumulated time
				mAccumulatedTime += mDeltaTime;

				while (mAccumulatedTime >= mTimeStepFixed)
				{
					mAccumulatedTime -= mTimeStepFixed;

					executeCallbacks(ExecutionStage::FixedUpdate, true);
				}
			}

			// Update
			{
				executeCallbacks(ExecutionStage::Update, true);
			}
		}

		// UI
		if (mShouldRenderEditorUi)
		{
			mUiManager->Update();
		}

		// Render
		{
			executeCallbacks(ExecutionStage::Render, true);
		}

		if (mPlayState == PlayState::JustStopped)
		{
			// Cleanup Systems and ECS
			executeCallbacks(ExecutionStage::Stop);

			ecsWorld->Reset();

			// Re-Initialize Systems and ECS
			//m_sceneData->Init();

			// Perform Pre-Gameplay Initialization on Systems
			executeCallbacks(ExecutionStage::Setup);

			audioSubsystem->stopAllSounds();

			mAccumulatedTime = 0.0;
			mPlayState = PlayState::Stopped;
		}

		auto windowSubsystem = getSubsystem<Window::WindowSubsystem>();
		if (windowSubsystem->ShouldPrimaryWindowClose())
		{
			mRunning = false;
		}

		return mRunning;
	}

	void Engine::destroy()
	{
		// Cleanup All Systems
		executeCallbacks(ExecutionStage::Cleanup);

		mSystems.clear();
		mSubsystems.clear();

		// Cleanup UI Manager
		if (mShouldRenderEditorUi)
		{
			mUiManager->Cleanup();
			mUiManager = nullptr;
		}

		ECS::EntityCache::Clear();

		// Clear Asset Registry
		assets::AssetRegistry::clear();
	}

	void Engine::addDefaultAssets()
	{
		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		UUID meshId1 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath1)->id();
		UUID meshId2 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath2)->id();
		UUID meshId3 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath3)->id();
		UUID meshId4 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath4)->id();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		UUID textureId1 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(texturePath1)->id();
		UUID textureId2 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(texturePath2)->id();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		UUID soundId1 = assets::AssetRegistry::get()->addAsset<assets::SoundAsset>(soundPath1)->id();
	}

	void Engine::reimportDefaultAssets()
	{
		//IO::ImportMesh("C:\\Projects\\PuffinProject\\model_backups\\chalet.obj");
		//IO::ImportMesh("C:\\Projects\\PuffinProject\\model_backups\\cube.obj");
		//IO::ImportMesh("C:\\Projects\\PuffinProject\\model_backups\\space_engineer.obj");
		//IO::ImportMesh("C:\\Projects\\PuffinProject\\model_backups\\Sphere.dae");
	}

	void Engine::loadAndResaveAssets()
	{
		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		UUID meshId1 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath1)->id();
		UUID meshId2 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath2)->id();
		UUID meshId3 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath3)->id();
		UUID meshId4 = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(meshPath4)->id();

		const auto meshAsset1 = std::static_pointer_cast<assets::StaticMeshAsset>(assets::AssetRegistry::get()->getAsset(meshId1));
		const auto meshAsset2 = std::static_pointer_cast<assets::StaticMeshAsset>(assets::AssetRegistry::get()->getAsset(meshId2));
		const auto meshAsset3 = std::static_pointer_cast<assets::StaticMeshAsset>(assets::AssetRegistry::get()->getAsset(meshId3));
		const auto meshAsset4 = std::static_pointer_cast<assets::StaticMeshAsset>(assets::AssetRegistry::get()->getAsset(meshId4));

		meshAsset1->load();
		meshAsset2->load();
		meshAsset3->load();
		meshAsset4->load();

		meshAsset1->save();
		meshAsset2->save();
		meshAsset3->save();
		meshAsset4->save();
	}

	void Engine::defaultScene()
	{
		auto ecsWorld = getSubsystem<ECS::World>();

		// Initialize Assets
		fs::path contentRootPath = assets::AssetRegistry::get()->contentRoot();

		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		UUID meshId1 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath1)->id();
		UUID meshId2 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath2)->id();
		UUID meshId3 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath3)->id();
		UUID meshId4 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath4)->id();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		UUID textureId1 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(texturePath1)->id();
		UUID textureId2 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(texturePath2)->id();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		UUID soundId1 = assets::AssetRegistry::get()->getAsset<assets::SoundAsset>(soundPath1)->id();

		const int numEntities = 7;
		std::vector<ECS::EntityPtr> entities;
		entities.reserve(numEntities);

		std::string names[numEntities] = { "House", "Sphere", "Falling Cube", "Dir Light", "Static Cube", "Plane", "Spot Light" };

		// Add Default Scene Components to ECS
		for (int i = 0; i < numEntities; i++)
		{
			const auto entity = ECS::CreateEntity(ecsWorld);
			entity->SetName(names[i]);
			entity->AddComponent<TransformComponent>();
			entity->AddComponent<rendering::MeshComponent>();
			entities.push_back(entity);
		}

		// Initialize Components with default values
		entities[0]->GetComponent<TransformComponent>() = { Vector3f(2.0f, 0.0f, 0.0f), maths::Quat(), Vector3f(1.0f) };
		entities[1]->GetComponent<TransformComponent>() = { Vector3f(-1.0f, 0.0f, 0.0f), maths::Quat(), Vector3f(1.0f) };
		entities[2]->GetComponent<TransformComponent>() = { Vector3f(0.0f, 0.0f, 0.0f), maths::Quat(), Vector3f(1.0f) };
		entities[3]->GetComponent<TransformComponent>() =
		{
			Vector3f(-5.0f, 0.0f, 0.0f),
			maths::Quat(.5f, -.5f, 0.0f),
			Vector3f(0.25f)
		};
		entities[4]->GetComponent<TransformComponent>() = { Vector3f(-1.75f, -5.0f, 0.0f), maths::Quat(), Vector3f(1.0f) };
		entities[5]->GetComponent<TransformComponent>() = { Vector3f(0.0f, -10.0f, 0.0f), maths::Quat(), Vector3f(10.0f, 1.0f, 10.0f) };
		entities[6]->GetComponent<TransformComponent>() =
		{
			Vector3f(5.0f, 0.0f, 0.0f),
			maths::Quat(-.5f, -.5f, 0.0f),
			Vector3f(0.25f)
		};

		entities[0]->GetComponent<rendering::MeshComponent>().meshAssetId = meshId1;
		entities[0]->GetComponent<rendering::MeshComponent>().textureAssetId = textureId1;

		entities[1]->GetComponent<rendering::MeshComponent>().meshAssetId = meshId2;
		entities[1]->GetComponent<rendering::MeshComponent>().textureAssetId = textureId2;

		entities[2]->GetComponent<rendering::MeshComponent>().meshAssetId = meshId3;
		entities[2]->GetComponent<rendering::MeshComponent>().textureAssetId = textureId2;
		entities[3]->GetComponent<rendering::MeshComponent>().meshAssetId = meshId3;
		entities[3]->GetComponent<rendering::MeshComponent>().textureAssetId = textureId2;
		entities[4]->GetComponent<rendering::MeshComponent>().meshAssetId = meshId3;
		entities[4]->GetComponent<rendering::MeshComponent>().textureAssetId = textureId2;
		entities[5]->GetComponent<rendering::MeshComponent>().meshAssetId = meshId3;
		entities[5]->GetComponent<rendering::MeshComponent>().textureAssetId = textureId2;
		entities[6]->GetComponent<rendering::MeshComponent>().meshAssetId = meshId3;
		entities[6]->GetComponent<rendering::MeshComponent>().textureAssetId = textureId2;

		// Setup Light Component
		entities[3]->AddComponent<rendering::LightComponent>();
		entities[3]->GetComponent<rendering::LightComponent>().color = Vector3f(1.f, 1.f, 1.f);
		entities[3]->GetComponent<rendering::LightComponent>().type = rendering::LightType::Directional;
		//entities[3]->AddComponent<Rendering::ShadowCasterComponent>();

		entities[6]->AddComponent<rendering::LightComponent>();
		entities[6]->GetComponent<rendering::LightComponent>().color = Vector3f(0.f, 0.f, 1.f);
		entities[6]->GetComponent<rendering::LightComponent>().type = rendering::LightType::Spot;

		auto& script = entities[0]->AddAndGetComponent<scripting::AngelScriptComponent>();
		script.name = "ExampleScript";
		script.dir = contentRootPath / "scripts\\Example.pscript";
	}

	void Engine::physicsScene()
	{
		// Initialize Assets
		fs::path contentRootPath = assets::AssetRegistry::get()->contentRoot();

		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		UUID meshId1 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath1)->id();
		UUID meshId2 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath2)->id();
		UUID meshId3 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath3)->id();
		UUID meshId4 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(meshPath4)->id();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		UUID textureId1 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(texturePath1)->id();
		UUID textureId2 = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(texturePath2)->id();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		UUID soundId1 = assets::AssetRegistry::get()->getAsset<assets::SoundAsset>(soundPath1)->id();

		auto enttSubsystem = getSubsystem<ECS::EnTTSubsystem>();
		auto registry = enttSubsystem->Registry();

		// Create Light Entity
		{
			const auto lightEntity = enttSubsystem->CreateEntity("Light");

			auto& transform = registry->emplace<TransformComponent>(lightEntity);
			transform.position = Vector3f(-5.0f, 0.0f, 0.0f);
			transform.rotation = maths::Quat(.5f, -0.5f, 0.0f);

			auto& light = registry->emplace<rendering::LightComponent>(lightEntity);
			light.type = rendering::LightType::Directional;
			light.color = glm::vec3(1.0f, 1.0f, 1.0f);
		}

		// Create Floor Entity
		{
			const auto floorEntity = enttSubsystem->CreateEntity("Floor");

			auto& transform = registry->emplace<TransformComponent>(floorEntity);
			transform.scale = Vector3f(250.0f, 1.0f, 1.0f);

			auto& mesh = registry->emplace<rendering::MeshComponent>(floorEntity);
			mesh.meshAssetId = meshId3;
			mesh.textureAssetId = textureId2;

			auto& box = registry->emplace<physics::BoxComponent2D>(floorEntity);
			registry->patch<physics::BoxComponent2D>(floorEntity, [](auto& box) { box.halfExtent = Vector2f(250.0f, 1.0f); });

			auto& rb = registry->emplace<physics::RigidbodyComponent2D>(floorEntity);
		}

		// Create Box Entities
		{
			const float xOffset = 20.0f;
			const Vector3f startPosition(-xOffset, 10.f, 0.f);
			const Vector3f endPosition(xOffset, 10.f, 0.f);

			const int numBodies = 10;
			Vector3f positionOffset = endPosition - startPosition;
			positionOffset.x /= numBodies;

			for (int i = 0; i < numBodies; i++)
			{
				const auto boxEntity = enttSubsystem->CreateEntity("Box " + i);

				Vector3f position = startPosition + (positionOffset * (float)i);

				auto& transform = registry->emplace<TransformComponent>(boxEntity);
				transform.position = position;

				auto& mesh = registry->emplace<rendering::MeshComponent>(boxEntity);
				mesh.meshAssetId = meshId3;
				mesh.textureAssetId = textureId2;

				auto& box = registry->emplace<physics::BoxComponent2D>(boxEntity);
				registry->patch<physics::BoxComponent2D>(boxEntity, [](auto& box) { box.halfExtent = Vector2f(1.0f); });

				auto& rb = registry->emplace<physics::RigidbodyComponent2D>(boxEntity);
				rb.mass = 1.0f;
				rb.bodyType = physics::BodyType::Dynamic;
			}
		}
	}

	void Engine::proceduralScene()
	{
		auto ecsWorld = getSubsystem<ECS::World>();

		// Initialize Assets
		fs::path contentRootPath = assets::AssetRegistry::get()->contentRoot();

		const fs::path& cubeMeshPath = "meshes\\cube.pstaticmesh";

		UUID cubeMeshId = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(cubeMeshPath)->id();

		const fs::path& cubeTexturePath = "textures\\cube.ptexture";

		UUID cubeTextureId = assets::AssetRegistry::get()->getAsset<assets::StaticMeshAsset>(cubeTexturePath)->id();

		const auto lightEntity = ECS::CreateEntity(ecsWorld);
		lightEntity->SetName("Light");
		lightEntity->AddComponent<TransformComponent>();
		lightEntity->GetComponent<TransformComponent>().position = { 0.0, 10.0, 0.0 };
		lightEntity->GetComponent<TransformComponent>().scale = { 0.25f };
		lightEntity->AddComponent<rendering::LightComponent>();
		lightEntity->GetComponent<rendering::LightComponent>().type = rendering::LightType::Directional;
		lightEntity->AddComponent<rendering::MeshComponent>();
		lightEntity->GetComponent<rendering::MeshComponent>().meshAssetId = cubeMeshId;
		lightEntity->GetComponent<rendering::MeshComponent>().textureAssetId = cubeTextureId;
		//lightEntity->AddComponent<Rendering::ShadowCasterComponent>();

		const auto planeEntity = ECS::CreateEntity(ecsWorld);
		planeEntity->SetName("Terrain");
		planeEntity->AddAndGetComponent<TransformComponent>().position = { 0.0, -10.0f, 0.0 };
		planeEntity->AddAndGetComponent<rendering::ProceduralMeshComponent>().textureAssetId = cubeTextureId;
		planeEntity->AddComponent<procedural::TerrainComponent>();
		planeEntity->GetComponent<procedural::TerrainComponent>().halfSize = { 50 };
		planeEntity->GetComponent<procedural::TerrainComponent>().numQuads = { 50 };
		planeEntity->GetComponent<procedural::TerrainComponent>().heightMultiplier = 10;

		const auto sphereEntity = ECS::CreateEntity(ecsWorld);
		sphereEntity->SetName("Sphere");
		sphereEntity->AddAndGetComponent<TransformComponent>().position = { 0.0, 5.0, 0.0 };
		sphereEntity->AddAndGetComponent<rendering::ProceduralMeshComponent>().textureAssetId = cubeTextureId;
		sphereEntity->AddComponent<procedural::IcoSphereComponent>();

		/*const auto boxEntity = ECS::CreateEntity(ecsWorld);
		boxEntity->SetName("Box");
		boxEntity->AddComponent<TransformComponent>();
		boxEntity->AddComponent<Rendering::MeshComponent>();
		boxEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = cubeMeshId;
		boxEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = cubeTextureId;*/
	}

	void Engine::play()
	{
		switch (mPlayState)
		{
		case PlayState::Stopped:
			mPlayState = PlayState::Started;
			break;
		case PlayState::Playing:
			mPlayState = PlayState::JustPaused;
			break;
		case PlayState::Paused:
			mPlayState = PlayState::JustUnpaused;
			break;
		}
	}

	void Engine::restart()
	{
		if (mPlayState == PlayState::Playing || mPlayState == PlayState::Paused || mPlayState == PlayState::Stopped)
		{
			mPlayState = PlayState::JustStopped;
		}
	}

	void Engine::exit()
	{
		mRunning = false;
	}
}