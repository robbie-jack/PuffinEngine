#include "Engine/Engine.hpp"

#include "ECS/ECS.h"
#include "ECS/Entity.hpp"

#include "Rendering/BGFX/BGFXRenderSystem.hpp"
#include "Rendering/Vulkan/VKRenderSystem.hpp"
#include "Physics/Box2D/Box2DPhysicsSystem.h"
#include "Physics/Onager2D/Onager2DPhysicsSystem.h"
#include "Scripting/AngelScriptSystem.h"
#include "Scripting/NativeScriptSystem.hpp"
#include "Procedural/ProceduralMeshGenSystem.hpp"

#include "Components/TransformComponent.h"
#include "Components/Scripting/AngelScriptComponent.hpp"
#include "Components/Scripting/NativeScriptComponent.hpp"
#include "Components/Procedural/ProceduralMeshComponent.hpp"

#include "Window/WindowSubsystem.hpp"
#include "Input/InputSubsystem.h"
#include "Engine/SignalSubsystem.hpp"
#include "Engine/EventSubsystem.hpp"
#include "Engine/EnkiTSSubsystem.hpp"
#include "ECS/EnTTSubsystem.hpp"

#include "SerializeScene.h"
#include "UI/Editor/UIManager.h"

#include "Audio/AudioSubsystem.h"

#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"
#include "Assets/TextureAsset.h"
#include "Assets/SoundAsset.h"

#include "Engine/JobSystem.hpp"

#include <chrono>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace Puffin::Core
{
	void Engine::init()
	{
		// Subsystems

		auto windowSubsystem = registerSubsystem<Window::WindowSubsystem>();
		auto eventSubsystem = registerSubsystem<EventSubsystem>();
		auto signalSubsystem = registerSubsystem<SignalSubsystem>();
		auto enkitsSubsystem = registerSubsystem<EnkiTSSubsystem>();
		auto inputSubsystem = registerSubsystem<Input::InputSubsystem>();
		auto audioSubsystem = registerSubsystem<Audio::AudioSubsystem>();
		auto ecsWorld = registerSubsystem<ECS::World>();
		auto enttSubsystem = registerSubsystem<ECS::EnTTSubsystem>();

		uiManager_ = std::make_shared<UI::UIManager>(shared_from_this());

		// Load Project File
		fs::path projectPath = fs::path("C:\\Projects\\PuffinProject\\Puffin.pproject");
		fs::path projectDirPath = projectPath;
		projectDirPath.remove_filename();

		IO::LoadProject(projectPath, projectFile_);

		// Load Default Scene (if set)
		fs::path defaultScenePath = projectDirPath.parent_path() / "content" / projectFile_.defaultScenePath;
		sceneData_ = std::make_shared<IO::SceneData>(ecsWorld, defaultScenePath);

		// Register Components to ECS World and Scene Data Class
		sceneData_->RegisterComponent<SceneObjectComponent>();
		sceneData_->RegisterComponent<TransformComponent>();

		sceneData_->RegisterComponent<Rendering::MeshComponent>();
		sceneData_->RegisterComponent<Rendering::LightComponent>();
		sceneData_->RegisterComponent<Rendering::ShadowCasterComponent>();
		sceneData_->RegisterComponent<Rendering::CameraComponent>();

		sceneData_->RegisterComponent<Physics::RigidbodyComponent2D>();
		sceneData_->RegisterComponent<Physics::BoxComponent2D>();
		sceneData_->RegisterComponent<Physics::CircleComponent2D>();

		sceneData_->RegisterComponent<Scripting::AngelScriptComponent>();
		sceneData_->RegisterComponent<Scripting::NativeScriptComponent>();

		sceneData_->RegisterComponent<Rendering::ProceduralMeshComponent>();
		sceneData_->RegisterComponent<Procedural::PlaneComponent>();
		sceneData_->RegisterComponent<Procedural::TerrainComponent>();
		sceneData_->RegisterComponent<Procedural::IcoSphereComponent>();

		// Systems
		//RegisterSystem<Rendering::BGFX::BGFXRenderSystem>();
		registerSystem<Rendering::VK::VKRenderSystem>();
		//RegisterSystem<Physics::Onager2DPhysicsSystem>();
		registerSystem<Physics::Box2DPhysicsSystem>();
		registerSystem<Scripting::AngelScriptSystem>();
		registerSystem<Scripting::NativeScriptSystem>();
		registerSystem<Procedural::ProceduralMeshGenSystem>();

		// Register Assets
		Assets::AssetRegistry::Get()->RegisterAssetType<Assets::StaticMeshAsset>();
		Assets::AssetRegistry::Get()->RegisterAssetType<Assets::TextureAsset>();
		Assets::AssetRegistry::Get()->RegisterAssetType<Assets::SoundAsset>();

		// Load Asset Cache
		Assets::AssetRegistry::Get()->ProjectName(projectFile_.name);
		Assets::AssetRegistry::Get()->ProjectRoot(projectDirPath);

		// Load Project Settings
		IO::LoadSettings(projectDirPath.parent_path() / "Settings.json", settings_);

		// Load/Initialize Assets
		//AddDefaultAssets();
		Assets::AssetRegistry::Get()->LoadAssetCache();
		//ReimportDefaultAssets();

		//Core::JobSystem::Get()->Start();

		// Create Default Scene in code -- used when scene serialization is changed
		//DefaultScene();
		physicsScene();
		//ProceduralScene();

		// Load Scene -- normal behaviour
		//m_sceneData->LoadAndInit();
		//m_sceneData->Save();

		running_ = true;
		playState_ = PlayState::stopped;

		// Initialize Systems
		{
			executeCallbacks(ExecutionStage::init);
			executeCallbacks(ExecutionStage::setup);
		}

		lastTime_ = glfwGetTime(); // Time Count Started
		currentTime_ = lastTime_;
	}

	bool Engine::update()
	{
		// Run Game Loop;
		lastTime_ = currentTime_;
		currentTime_ = glfwGetTime();
		deltaTime_ = currentTime_ - lastTime_;

		updateExecutionTime();

		if (frameRateMax_ > 0)
		{
			const double deltaTimeMax = 1.0 / frameRateMax_;
			double idleStartTime = 0.0, idleEndTime = 0.0;

			if (shouldTrackExecutionTime_)
			{
				// Sleep until next frame should start
				idleStartTime = glfwGetTime();
			}

			while (deltaTime_ < deltaTimeMax)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(0));

				currentTime_ = glfwGetTime();
				deltaTime_ = currentTime_ - lastTime_;
			}

			if (shouldTrackExecutionTime_)
			{
				idleEndTime = glfwGetTime();

				stageExecutionTime_[ExecutionStage::idle] = idleEndTime - idleStartTime;
			}
		}

		// Make sure delta time never exceeds 1/30th of a second
		if (deltaTime_ > timeStepLimit_)
		{
			deltaTime_ = timeStepLimit_;
		}

		auto ecsWorld = getSubsystem<ECS::World>();
		auto audioSubsystem = getSubsystem<Audio::AudioSubsystem>();

		// Update all Subsystems
		{
			executeCallbacks(ExecutionStage::subsystemUpdate, true);
		}

		auto inputSubsystem = getSubsystem<Input::InputSubsystem>();
		if (inputSubsystem->GetAction("Play").state == Input::KeyState::PRESSED)
		{
			play();
		}

		if (inputSubsystem->GetAction("Restart").state == Input::KeyState::PRESSED)
		{
			restart();
		}

		// Call system start functions to prepare for gameplay
		if (playState_ == PlayState::started)
		{
			executeCallbacks(ExecutionStage::start);

			// Get Snapshot of current scene data
			//m_sceneData->UpdateData();

			accumulatedTime_ = 0.0;
			playState_ = PlayState::playing;
		}

		if (playState_ == PlayState::justPaused)
		{
			audioSubsystem->PauseAllSounds();

			playState_ = PlayState::paused;
		}

		if (playState_ == PlayState::justUnpaused)
		{
			audioSubsystem->PlayAllSounds();

			playState_ = PlayState::playing;
		}

		if (playState_ == PlayState::playing)
		{
			// Fixed Update
			{
				// Add onto accumulated time
				accumulatedTime_ += deltaTime_;

				while (accumulatedTime_ >= timeStepFixed_)
				{
					accumulatedTime_ -= timeStepFixed_;

					executeCallbacks(ExecutionStage::fixedUpdate, true);
				}
			}

			// Update
			{
				executeCallbacks(ExecutionStage::update, true);
			}
		}

		// UI
		if (shouldRenderEditorUi_)
		{
			uiManager_->Update();
		}

		// Render
		{
			executeCallbacks(ExecutionStage::render, true);
		}

		if (playState_ == PlayState::justStopped)
		{
			// Cleanup Systems and ECS
			executeCallbacks(ExecutionStage::stop);

			ecsWorld->Reset();

			// Re-Initialize Systems and ECS
			//m_sceneData->Init();

			// Perform Pre-Gameplay Initialization on Systems
			executeCallbacks(ExecutionStage::setup);

			audioSubsystem->StopAllSounds();

			accumulatedTime_ = 0.0;
			playState_ = PlayState::stopped;
		}

		auto windowSubsystem = getSubsystem<Window::WindowSubsystem>();
		if (windowSubsystem->ShouldPrimaryWindowClose())
		{
			running_ = false;
		}

		return running_;
	}

	void Engine::destroy()
	{
		// Cleanup All Systems
		executeCallbacks(ExecutionStage::cleanup);

		systems_.clear();
		subsystems_.clear();

		// Cleanup UI Manager
		if (shouldRenderEditorUi_)
		{
			uiManager_->Cleanup();
			uiManager_ = nullptr;
		}

		while (true)
		{
			if (JobSystem::Get()->Wait())
			{
				break;
			}
		}

		/*JobSystem::Get()->Stop();
		JobSystem::Clear();*/

		ECS::EntityCache::Clear();

		// Clear Asset Registry
		Assets::AssetRegistry::Clear();
	}

	void Engine::addDefaultAssets()
	{
		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		UUID meshId1 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath1)->ID();
		UUID meshId2 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath2)->ID();
		UUID meshId3 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath3)->ID();
		UUID meshId4 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath4)->ID();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		UUID textureId1 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(texturePath1)->ID();
		UUID textureId2 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(texturePath2)->ID();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		UUID soundId1 = Assets::AssetRegistry::Get()->AddAsset<Assets::SoundAsset>(soundPath1)->ID();
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

		UUID meshId1 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath1)->ID();
		UUID meshId2 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath2)->ID();
		UUID meshId3 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath3)->ID();
		UUID meshId4 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath4)->ID();

		const auto meshAsset1 = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(meshId1));
		const auto meshAsset2 = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(meshId2));
		const auto meshAsset3 = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(meshId3));
		const auto meshAsset4 = std::static_pointer_cast<Assets::StaticMeshAsset>(Assets::AssetRegistry::Get()->GetAsset(meshId4));

		meshAsset1->Load();
		meshAsset2->Load();
		meshAsset3->Load();
		meshAsset4->Load();

		meshAsset1->Save();
		meshAsset2->Save();
		meshAsset3->Save();
		meshAsset4->Save();
	}

	void Engine::defaultScene()
	{
		auto ecsWorld = getSubsystem<ECS::World>();

		// Initialize Assets
		fs::path contentRootPath = Assets::AssetRegistry::Get()->ContentRoot();

		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		UUID meshId1 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath1)->ID();
		UUID meshId2 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath2)->ID();
		UUID meshId3 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath3)->ID();
		UUID meshId4 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath4)->ID();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		UUID textureId1 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(texturePath1)->ID();
		UUID textureId2 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(texturePath2)->ID();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		UUID soundId1 = Assets::AssetRegistry::Get()->GetAsset<Assets::SoundAsset>(soundPath1)->ID();

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
			entity->AddComponent<Rendering::MeshComponent>();
			entities.push_back(entity);
		}

		// Initialize Components with default values
		entities[0]->GetComponent<TransformComponent>() = { Vector3f(2.0f, 0.0f, 0.0f), Maths::Quat(), Vector3f(1.0f) };
		entities[1]->GetComponent<TransformComponent>() = { Vector3f(-1.0f, 0.0f, 0.0f), Maths::Quat(), Vector3f(1.0f) };
		entities[2]->GetComponent<TransformComponent>() = { Vector3f(0.0f, 0.0f, 0.0f), Maths::Quat(), Vector3f(1.0f) };
		entities[3]->GetComponent<TransformComponent>() =
		{
			Vector3f(-5.0f, 0.0f, 0.0f),
			Maths::Quat(.5f, -.5f, 0.0f),
			Vector3f(0.25f)
		};
		entities[4]->GetComponent<TransformComponent>() = { Vector3f(-1.75f, -5.0f, 0.0f), Maths::Quat(), Vector3f(1.0f) };
		entities[5]->GetComponent<TransformComponent>() = { Vector3f(0.0f, -10.0f, 0.0f), Maths::Quat(), Vector3f(10.0f, 1.0f, 10.0f) };
		entities[6]->GetComponent<TransformComponent>() =
		{
			Vector3f(5.0f, 0.0f, 0.0f),
			Maths::Quat(-.5f, -.5f, 0.0f),
			Vector3f(0.25f)
		};

		entities[0]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId1;
		entities[0]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId1;

		entities[1]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId2;
		entities[1]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;

		entities[2]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		entities[2]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;
		entities[3]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		entities[3]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;
		entities[4]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		entities[4]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;
		entities[5]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		entities[5]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;
		entities[6]->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		entities[6]->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;

		// Setup Light Component
		entities[3]->AddComponent<Rendering::LightComponent>();
		entities[3]->GetComponent<Rendering::LightComponent>().color = Vector3f(1.f, 1.f, 1.f);
		entities[3]->GetComponent<Rendering::LightComponent>().type = Rendering::LightType::DIRECTIONAL;
		//entities[3]->AddComponent<Rendering::ShadowCasterComponent>();

		entities[6]->AddComponent<Rendering::LightComponent>();
		entities[6]->GetComponent<Rendering::LightComponent>().color = Vector3f(0.f, 0.f, 1.f);
		entities[6]->GetComponent<Rendering::LightComponent>().type = Rendering::LightType::SPOT;

		auto& script = entities[0]->AddAndGetComponent<Scripting::AngelScriptComponent>();
		script.name = "ExampleScript";
		script.dir = contentRootPath / "scripts\\Example.pscript";
	}

	void Engine::physicsScene()
	{
		// Initialize Assets
		fs::path contentRootPath = Assets::AssetRegistry::Get()->ContentRoot();

		const fs::path& meshPath1 = "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = "meshes\\space_engineer.pstaticmesh";

		UUID meshId1 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath1)->ID();
		UUID meshId2 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath2)->ID();
		UUID meshId3 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath3)->ID();
		UUID meshId4 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(meshPath4)->ID();

		const fs::path& texturePath1 = "textures\\chalet.ptexture";
		const fs::path& texturePath2 = "textures\\cube.ptexture";

		UUID textureId1 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(texturePath1)->ID();
		UUID textureId2 = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(texturePath2)->ID();

		const fs::path& soundPath1 = "sounds\\Select 1.wav";

		UUID soundId1 = Assets::AssetRegistry::Get()->GetAsset<Assets::SoundAsset>(soundPath1)->ID();

		auto enttSubsystem = getSubsystem<ECS::EnTTSubsystem>();
		auto registry = enttSubsystem->Registry();

		// Create Light Entity
		{
			const auto lightEntity = enttSubsystem->CreateEntity("Light");

			auto& transform = registry->emplace<TransformComponent>(lightEntity);
			transform.position = Vector3f(-5.0f, 0.0f, 0.0f);
			transform.rotation = Maths::Quat(.5f, -0.5f, 0.0f);

			auto& light = registry->emplace<Rendering::LightComponent>(lightEntity);
			light.type = Rendering::LightType::DIRECTIONAL;
			light.color = glm::vec3(1.0f, 1.0f, 1.0f);
		}

		// Create Floor Entity
		{
			const auto floorEntity = enttSubsystem->CreateEntity("Floor");

			auto& transform = registry->emplace<TransformComponent>(floorEntity);
			transform.scale = Vector3f(250.0f, 1.0f, 1.0f);

			auto& mesh = registry->emplace<Rendering::MeshComponent>(floorEntity);
			mesh.meshAssetID = meshId3;
			mesh.textureAssetID = textureId2;

			auto& box = registry->emplace<Physics::BoxComponent2D>(floorEntity);
			registry->patch<Physics::BoxComponent2D>(floorEntity, [](auto& box) { box.halfExtent = Vector2f(250.0f, 1.0f); });

			auto& rb = registry->emplace<Physics::RigidbodyComponent2D>(floorEntity);
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

				auto& mesh = registry->emplace<Rendering::MeshComponent>(boxEntity);
				mesh.meshAssetID = meshId3;
				mesh.textureAssetID = textureId2;

				auto& box = registry->emplace<Physics::BoxComponent2D>(boxEntity);
				registry->patch<Physics::BoxComponent2D>(boxEntity, [](auto& box) { box.halfExtent = Vector2f(1.0f); });

				auto& rb = registry->emplace<Physics::RigidbodyComponent2D>(boxEntity);
				rb.mass = 1.0f;
				rb.bodyType = Physics::BodyType::Dynamic;
			}
		}
	}

	void Engine::proceduralScene()
	{
		auto ecsWorld = getSubsystem<ECS::World>();

		// Initialize Assets
		fs::path contentRootPath = Assets::AssetRegistry::Get()->ContentRoot();

		const fs::path& cubeMeshPath = "meshes\\cube.pstaticmesh";

		UUID cubeMeshId = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(cubeMeshPath)->ID();

		const fs::path& cubeTexturePath = "textures\\cube.ptexture";

		UUID cubeTextureId = Assets::AssetRegistry::Get()->GetAsset<Assets::StaticMeshAsset>(cubeTexturePath)->ID();

		const auto lightEntity = ECS::CreateEntity(ecsWorld);
		lightEntity->SetName("Light");
		lightEntity->AddComponent<TransformComponent>();
		lightEntity->GetComponent<TransformComponent>().position = { 0.0, 10.0, 0.0 };
		lightEntity->GetComponent<TransformComponent>().scale = { 0.25f };
		lightEntity->AddComponent<Rendering::LightComponent>();
		lightEntity->GetComponent<Rendering::LightComponent>().type = Rendering::LightType::DIRECTIONAL;
		lightEntity->AddComponent<Rendering::MeshComponent>();
		lightEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = cubeMeshId;
		lightEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = cubeTextureId;
		//lightEntity->AddComponent<Rendering::ShadowCasterComponent>();

		const auto planeEntity = ECS::CreateEntity(ecsWorld);
		planeEntity->SetName("Terrain");
		planeEntity->AddAndGetComponent<TransformComponent>().position = { 0.0, -10.0f, 0.0 };
		planeEntity->AddAndGetComponent<Rendering::ProceduralMeshComponent>().textureAssetID = cubeTextureId;
		planeEntity->AddComponent<Procedural::TerrainComponent>();
		planeEntity->GetComponent<Procedural::TerrainComponent>().halfSize = { 50 };
		planeEntity->GetComponent<Procedural::TerrainComponent>().numQuads = { 50 };
		planeEntity->GetComponent<Procedural::TerrainComponent>().heightMultiplier = 10;

		const auto sphereEntity = ECS::CreateEntity(ecsWorld);
		sphereEntity->SetName("Sphere");
		sphereEntity->AddAndGetComponent<TransformComponent>().position = { 0.0, 5.0, 0.0 };
		sphereEntity->AddAndGetComponent<Rendering::ProceduralMeshComponent>().textureAssetID = cubeTextureId;
		sphereEntity->AddComponent<Procedural::IcoSphereComponent>();

		/*const auto boxEntity = ECS::CreateEntity(ecsWorld);
		boxEntity->SetName("Box");
		boxEntity->AddComponent<TransformComponent>();
		boxEntity->AddComponent<Rendering::MeshComponent>();
		boxEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = cubeMeshId;
		boxEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = cubeTextureId;*/
	}

	void Engine::play()
	{
		switch (playState_)
		{
		case PlayState::stopped:
			playState_ = PlayState::started;
			break;
		case PlayState::playing:
			playState_ = PlayState::justPaused;
			break;
		case PlayState::paused:
			playState_ = PlayState::justUnpaused;
			break;
		}
	}

	void Engine::restart()
	{
		if (playState_ == PlayState::playing || playState_ == PlayState::paused || playState_ == PlayState::stopped)
		{
			playState_ = PlayState::justStopped;
		}
	}

	void Engine::exit()
	{
		running_ = false;
	}
}