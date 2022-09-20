#include "Engine/Engine.hpp"

#include "ECS/ECS.h"
#include "ECS/Entity.h"

#include "Rendering/VulkanEngine.h"
#include "Physics/Box2D/Box2DPhysicsSystem.h"
#include "Scripting/AngelScriptSystem.h"

#include "Components/TransformComponent.h"
#include "Components/AngelScriptComponent.h"

#include "Types/ComponentFlags.h"

#include "Input/InputSubsystem.h"
#include "Engine/EventSubsystem.hpp"

#include "SerializeScene.h"
#include "UI/UIManager.h"

#include "Audio/AudioSubsystem.h"

#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"
#include "Assets/TextureAsset.h"
#include "Assets/SoundAsset.h"

#include "Assets/Importers/ModelImporter.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace Puffin::Core
{
	const int WIDTH = 1280;
	const int HEIGHT = 720;

	void Engine::Init()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		m_window = glfwCreateWindow(1280, 720, "Puffin Engine", m_monitor, nullptr);

		// Subsystems
		auto ecsWorld = RegisterSubsystem<ECS::World>();
		auto eventSubsystem = RegisterSubsystem<Core::EventSubsystem>();
		auto inputSubsystem = RegisterSubsystem<Input::InputSubsystem>();
		auto audioSubsystem = RegisterSubsystem<Audio::AudioSubsystem>();

		m_uiManager = std::make_shared<UI::UIManager>(shared_from_this());

		// Systems
		std::shared_ptr<Rendering::VulkanEngine> vulkanEngine = RegisterSystem<Rendering::VulkanEngine>();
		std::shared_ptr<Physics::Box2DPhysicsSystem> physicsSystem = RegisterSystem<Physics::Box2DPhysicsSystem>();
		std::shared_ptr<Scripting::AngelScriptSystem> scriptingSystem = RegisterSystem<Scripting::AngelScriptSystem>();

		// Register Components to ECS World
		ecsWorld->RegisterComponent<TransformComponent>();
		ecsWorld->RegisterComponent<InterpolatedTransformComponent>();
		ecsWorld->RegisterComponent<Rendering::MeshComponent>();
		ecsWorld->RegisterComponent<Rendering::PointLightComponent>();
		ecsWorld->RegisterComponent<Rendering::DirectionalLightComponent>();
		ecsWorld->RegisterComponent<Rendering::SpotLightComponent>();
		ecsWorld->RegisterComponent<Rendering::ShadowCasterComponent>();
		ecsWorld->RegisterComponent<Physics::Box2DRigidbodyComponent>();
		ecsWorld->RegisterComponent<Physics::Box2DBoxComponent>();
		ecsWorld->RegisterComponent<Physics::Box2DCircleComponent>();
		ecsWorld->RegisterComponent<Scripting::AngelScriptComponent>();

		ecsWorld->AddComponentDependencies<Rendering::MeshComponent, TransformComponent>();
		ecsWorld->AddComponentDependencies<Rendering::LightComponent, TransformComponent>();
		ecsWorld->AddComponentDependencies<Physics::Box2DRigidbodyComponent, TransformComponent>();
		ecsWorld->AddComponentDependencies<Physics::Box2DRigidbodyComponent, InterpolatedTransformComponent>();

		// Register Entity Flags

		// Register Component Flags
		ecsWorld->RegisterComponentFlag<FlagDirty>(true);
		ecsWorld->RegisterComponentFlag<FlagDeleted>();

		// Load Project File
		fs::path projectPath = fs::path("D:\\Projects\\PuffinProject\\Puffin.pproject");
		fs::path projectDirPath = projectPath;
		projectDirPath.remove_filename();

		IO::LoadProject(projectPath, projectFile);

		// Register Assets
		Assets::AssetRegistry::Get()->RegisterAssetType<Assets::StaticMeshAsset>();
		Assets::AssetRegistry::Get()->RegisterAssetType<Assets::TextureAsset>();
		Assets::AssetRegistry::Get()->RegisterAssetType<Assets::SoundAsset>();

		// Load Asset Cache
		Assets::AssetRegistry::Get()->ProjectName(projectFile.name);
		Assets::AssetRegistry::Get()->ProjectRoot(projectDirPath);

		// Load Project Settings
		IO::LoadSettings(projectDirPath.parent_path() / "Settings.json", settings);

		// Load Default Scene (if set)
		fs::path defaultScenePath = projectDirPath.parent_path() / "content" / projectFile.defaultScenePath;
		m_sceneData = std::make_shared<IO::SceneData>(ecsWorld, defaultScenePath);

		// Register Components to Scene Data
		m_sceneData->RegisterComponent<TransformComponent>("Transforms");
		m_sceneData->RegisterComponent<Rendering::MeshComponent>("Meshes");
		m_sceneData->RegisterComponent<Rendering::PointLightComponent>("PointLights");
		m_sceneData->RegisterComponent<Rendering::DirectionalLightComponent>("DirectionalLights");
		m_sceneData->RegisterComponent<Rendering::SpotLightComponent>("SpotLights");
		m_sceneData->RegisterComponent<Rendering::ShadowCasterComponent>("Shadowcasters");
		m_sceneData->RegisterComponent<Physics::Box2DRigidbodyComponent>("Rigidbodies");
		m_sceneData->RegisterComponent<Physics::Box2DBoxComponent>("Boxes");
		m_sceneData->RegisterComponent<Physics::Box2DCircleComponent>("Circles");
		m_sceneData->RegisterComponent<Scripting::AngelScriptComponent>("Scripts");

		// Load/Initialize Assets
		//AddDefaultAssets();
		Assets::AssetRegistry::Get()->LoadAssetCache();

		// Create Default Scene in code -- used when scene serialization is changed
		//DefaultScene(ecsWorld);
		//PhysicsScene(ecsWorld);

		// Load Scene -- normal behaviour
		m_sceneData->LoadAndInit();

		running = true;
		playState = PlayState::STOPPED;

		// Initialize Subsystems
		for (auto& [fst, snd] : m_subsystems)
		{
			snd->Init();
		}

		// Initialize Systems
		for (auto& system : m_systems)
		{
			system->Init();
			system->PreStart();
		}

		m_lastTime = std::chrono::high_resolution_clock::now(); // Time Count Started
		m_currentTime = std::chrono::high_resolution_clock::now();
		m_accumulatedTime = 0.0;
		m_timeStep = 1 / 60.0; // How often deterministic code like physics should occur (defaults to 60 times a second)
		m_maxTimeStep = 1 / 30.0;
	}

	bool Engine::Update()
	{
		// Run Game Loop;
		m_lastTime = m_currentTime;
		m_currentTime = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> duration = m_currentTime - m_lastTime;
		m_deltaTime = duration.count();

		// Make sure delta time never exceeds 1/30th of a second
		if (m_deltaTime > m_maxTimeStep)
		{
			m_deltaTime = m_maxTimeStep;
		}

		auto ecsWorld = GetSubsystem<ECS::World>();
		auto audioSubsystem = GetSubsystem<Audio::AudioSubsystem>();

		// Update all Subsystems
		for (auto& [fst, snd] : m_subsystems)
		{
			if (snd->ShouldUpdate())
			{
				snd->Update();
			}
		}

		// Call system start functions to prepare for gameplay
		if (playState == PlayState::STARTED)
		{
			for (auto& system : m_systems)
			{
				system->Start();
			}

			// Get Snapshot of current scene data
			m_sceneData->UpdateData();

			m_accumulatedTime = 0.0;
			playState = PlayState::PLAYING;
		}

		if (playState == PlayState::JUST_PAUSED)
		{
			audioSubsystem->PauseAllSounds();

			playState = PlayState::PAUSED;
		}

		if (playState == PlayState::JUST_UNPAUSED)
		{
			audioSubsystem->PlayAllSounds();

			playState = PlayState::PLAYING;
		}

		// Fixed Update
		if (playState == PlayState::PLAYING)
		{
			// Add onto accumulated time
			m_accumulatedTime += m_deltaTime;

			auto stageStartTime = std::chrono::high_resolution_clock::now();

			// Perform system updates until simulation is caught up
			while (m_accumulatedTime >= m_timeStep)
			{
				m_accumulatedTime -= m_timeStep;

				// FixedUpdate Systems
				for (auto& system : m_systemUpdateVectors[Core::UpdateOrder::FixedUpdate])
				{
					auto startTime = std::chrono::high_resolution_clock::now();

					system->Update();

					auto endTime = std::chrono::high_resolution_clock::now();

					const std::chrono::duration<double> systemDuration = endTime - startTime;
					m_systemExecutionTime[Core::UpdateOrder::FixedUpdate][system->GetInfo().name] = systemDuration.count();
				}
			}

			auto stageEndTime = std::chrono::high_resolution_clock::now();

			const std::chrono::duration<double> stageDuration = stageEndTime - stageStartTime;
			m_stageExecutionTime[Core::UpdateOrder::FixedUpdate] = stageDuration.count();
		}

		// Update
		if (playState == PlayState::PLAYING)
		{
			auto stageStartTime = std::chrono::high_resolution_clock::now();

			for (auto& system : m_systemUpdateVectors[Core::UpdateOrder::Update])
			{
				auto startTime = std::chrono::high_resolution_clock::now();

				system->Update();

				auto endTime = std::chrono::high_resolution_clock::now();

				const std::chrono::duration<double> systemDuration = endTime - startTime;
				m_systemExecutionTime[Core::UpdateOrder::Update][system->GetInfo().name] = systemDuration.count();
			}

			auto stageEndTime = std::chrono::high_resolution_clock::now();

			const std::chrono::duration<double> stageDuration = stageEndTime - stageStartTime;
			m_stageExecutionTime[Core::UpdateOrder::Update] = stageDuration.count();
		}

		// UI
		m_uiManager->Update();

		auto stageStartTime = std::chrono::high_resolution_clock::now();

		// Rendering
		for (auto system : m_systemUpdateVectors[Core::UpdateOrder::Rendering])
		{
			auto startTime = std::chrono::high_resolution_clock::now();

			system->Update();

			auto endTime = std::chrono::high_resolution_clock::now();

			const std::chrono::duration<double> systemDuration = endTime - startTime;
			m_systemExecutionTime[Core::UpdateOrder::Rendering][system->GetInfo().name] = systemDuration.count();
		}

		auto stageEndTime = std::chrono::high_resolution_clock::now();

		const std::chrono::duration<double> stageDuration = stageEndTime - stageStartTime;
		m_stageExecutionTime[Core::UpdateOrder::Rendering] = stageDuration.count();

		if (playState == PlayState::JUST_STOPPED)
		{
			// Cleanup Systems and ECS
			for (auto system : m_systems)
			{
				system->Stop();
			}
			ecsWorld->Reset();

			// Re-Initialize Systems and ECS
			m_sceneData->Init();

			// Perform Pre-Gameplay Initiualization on Systems
			for (auto system : m_systems)
			{
				system->PreStart();
			}

			audioSubsystem->StopAllSounds();

			m_accumulatedTime = 0.0;
			playState = PlayState::STOPPED;
		}

		if (glfwWindowShouldClose(m_window))
		{
			running = false;
		}

		return running;
	}

	void Engine::Destroy()
	{
		auto ecsWorld = GetSubsystem<ECS::World>();

		// Cleanup All Systems
		for (auto system : m_systems)
		{
			system->Cleanup();
		}

		for (auto& [fst, snd] : m_subsystems)
		{
			snd->Destroy();
			snd = nullptr;
		}

		m_subsystems.clear();

		m_uiManager->Cleanup();
		m_uiManager = nullptr;

		Assets::AssetRegistry::Clear();

		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	void Engine::AddDefaultAssets()
	{
		IO::ImportMesh("D:\\Projects\\PuffinProject\\model_backups\\chalet.obj");
		IO::ImportMesh("D:\\Projects\\PuffinProject\\model_backups\\cube.obj");
		IO::ImportMesh("D:\\Projects\\PuffinProject\\model_backups\\space_engineer.obj");
		IO::ImportMesh("D:\\Projects\\PuffinProject\\model_backups\\Sphere.dae");

		//IO::ImportTexture("D:\\Projects\\PuffinProject\\texture_backups\\chalet.jpg");
		//IO::ImportTexture("D:\\Projects\\PuffinProject\\texture_backups\\cube.png");

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

	void Engine::DefaultScene(std::shared_ptr<ECS::World> world)
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

		// Initialize EntityManager with Existing Entities
		world->InitEntitySystem();

		const int numEntities = 7;
		std::vector<std::shared_ptr<ECS::Entity>> entities;
		entities.reserve(numEntities);

		// Add Default Scene Components to ECS
		for (int i = 0; i < numEntities; i++)
		{
			const auto entity = ECS::CreateEntity(world);
			entity->AddComponent<TransformComponent>();
			entity->AddComponent<Rendering::MeshComponent>();
			entities.push_back(entity);
		}

		world->SetEntityName(1, "House");
		world->SetEntityName(2, "Sphere");
		world->SetEntityName(3, "Falling Cube");
		world->SetEntityName(4, "Light");
		world->SetEntityName(5, "Static Cube");
		world->SetEntityName(6, "Plane");
		world->SetEntityName(7, "Light 2");

		entities[3]->AddComponent<Rendering::SpotLightComponent>();
		entities[6]->AddComponent<Rendering::SpotLightComponent>();

		// Initialize Components with default values
		entities[0]->GetComponent<TransformComponent>() = {Vector3f(2.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 0.0f), Vector3f(1.0f)};
		entities[1]->GetComponent<TransformComponent>() = { Vector3f(-1.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 0.0f), Vector3f(1.0f) };
		entities[2]->GetComponent<TransformComponent>() = { Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 0.0f), Vector3f(1.0f) };
		entities[3]->GetComponent<TransformComponent>() = { Vector3f(-10.0f, 0.0f, 2.0f), Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.25f) };
		entities[4]->GetComponent<TransformComponent>() = { Vector3f(-1.75f, -5.0f, 0.0f), Vector3f(0.0f, 0.0f, 0.0f), Vector3f(1.0f) };
		entities[5]->GetComponent<TransformComponent>() = { Vector3f(0.0f, -10.0f, 0.0f), Vector3f(0.0f, 0.0f, 0.0f), Vector3f(10.0f, 1.0f, 10.0f) };
		entities[6]->GetComponent<TransformComponent>() = { Vector3f(5.0f, 0.0f, 2.0f), Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.25f) };

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

		entities[3]->GetComponent<Rendering::SpotLightComponent>().direction = glm::vec3(1.0f, -1.0f, 0.0f);
		entities[3]->AddComponent<Rendering::ShadowCasterComponent>();

		entities[6]->GetComponent<Rendering::SpotLightComponent>().direction = glm::vec3(-1.0f, -1.0f, 0.0f);
		entities[6]->GetComponent<Rendering::SpotLightComponent>().diffuseColor = glm::vec3(0.25f, 0.25f, 1.0f);

		auto& script = entities[0]->AddAndGetComponent<Scripting::AngelScriptComponent>();
		script.name = "ExampleScript";
		script.dir = contentRootPath / "scripts\\Example.pscript";
	}

	void Engine::PhysicsScene(std::shared_ptr<ECS::World> world)
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

		world->InitEntitySystem();

		// Create Light Entity
		const auto lightEntity = ECS::CreateEntity(world);

		lightEntity->SetName("Light");

		lightEntity->AddAndGetComponent<TransformComponent>() = { Vector3f(0.0f, 10.0f, 0.0f), Vector3f(0.0f), Vector3f(1.0f) };

		auto& lightComp = lightEntity->AddAndGetComponent<Rendering::DirectionalLightComponent>();
		lightComp.direction = glm::vec3(1.0f, -1.0f, 0.0f);
		lightComp.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
		lightComp.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);

		// Create Box Entity
		const auto boxEntity = ECS::CreateEntity(world);

		boxEntity->SetName("Box");

		boxEntity->AddComponent<TransformComponent>();
		boxEntity->AddComponent<Rendering::MeshComponent>();
		boxEntity->AddComponent<Physics::Box2DRigidbodyComponent>();
		boxEntity->AddComponent<Physics::Box2DBoxComponent>();
		boxEntity->AddComponent<Scripting::AngelScriptComponent>();

		boxEntity->GetComponent<TransformComponent>() = { Vector3f(-2.0f, 10.0f, 0.0f), Vector3f(0.0f), Vector3f(1.0f) };

		boxEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		boxEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;

		boxEntity->GetComponent<Physics::Box2DRigidbodyComponent>().bodyDef.type = b2_dynamicBody;

		boxEntity->GetComponent<Scripting::AngelScriptComponent>().name = "PhysicsScript";
		boxEntity->GetComponent<Scripting::AngelScriptComponent>().dir = contentRootPath / "scripts\\Physics.pscript";

		// Create Circle Entity
		const auto circleEntity = ECS::CreateEntity(world);

		circleEntity->SetName("Circle");

		circleEntity->AddComponent<TransformComponent>();
		circleEntity->AddComponent<Rendering::MeshComponent>();
		circleEntity->AddComponent<Physics::Box2DRigidbodyComponent>();
		circleEntity->AddComponent<Physics::Box2DCircleComponent>();

		circleEntity->GetComponent<TransformComponent>() = { Vector3f(2.0f, 10.0f, 0.0f), Vector3f(0.0f), Vector3f(1.0f) };

		circleEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId2;
		circleEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;

		circleEntity->GetComponent<Physics::Box2DRigidbodyComponent>().bodyDef.type = b2_dynamicBody;

		// Create Floor Entity
		const auto floorEntity = ECS::CreateEntity(world);

		floorEntity->SetName("Floor");

		floorEntity->AddComponent<TransformComponent>();
		floorEntity->AddComponent<Rendering::MeshComponent>();
		floorEntity->AddComponent<Physics::Box2DRigidbodyComponent>();
		floorEntity->AddComponent<Physics::Box2DBoxComponent>();

		floorEntity->GetComponent<TransformComponent>() = { Vector3f(0.0f), Vector3f(0.0f), Vector3f(5.0f, 1.0f, 1.0f) };

		floorEntity->GetComponent<Rendering::MeshComponent>().meshAssetID = meshId3;
		floorEntity->GetComponent<Rendering::MeshComponent>().textureAssetID = textureId2;

		floorEntity->GetComponent<Physics::Box2DRigidbodyComponent>().bodyDef.type = b2_staticBody;
		floorEntity->GetComponent<Physics::Box2DBoxComponent>().data.halfExtent = Vector2f(5.0f, 1.0f);
	}

	void Engine::Play()
	{
		switch (playState)
		{
		case PlayState::STOPPED:
			playState = PlayState::STARTED;
			break;
		case PlayState::PLAYING:
			playState = PlayState::JUST_PAUSED;
			break;
		case PlayState::PAUSED:
			playState = PlayState::JUST_UNPAUSED;
			break;
		}
	}

	void Engine::Restart()
	{
		if (playState == PlayState::PLAYING || playState == PlayState::PAUSED || playState == PlayState::STOPPED)
		{
			playState = PlayState::JUST_STOPPED;
		}
	}

	void Engine::Exit()
	{
		running = false;
	}
}