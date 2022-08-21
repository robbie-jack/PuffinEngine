#include "Engine.h"

#include "ECS/ECS.h"
#include "ECS/Entity.h"

#include "Rendering/VulkanEngine.h"
//#include "Physics/PhysicsSystem2D.h"
#include "Physics/Box2D/Box2DPhysicsSystem.h"
#include "Scripting/AngelScriptSystem.h"

#include "Components/AngelScriptComponent.h"
#include "Components/TransformComponent.h"

#include "Types/ComponentFlags.h"

#include "Input/InputManager.h"

#include "SerializeScene.h"
#include "UI/UIManager.h"

#include "Audio/AudioManager.h"

#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"
#include "Assets/TextureAsset.h"
#include "Assets/SoundAsset.h"

#include <chrono>

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


	}

	void Engine::MainLoop()
	{
		// Managers/ECS World
		std::shared_ptr<ECS::World> ECSWorld = std::make_shared<ECS::World>();
		std::shared_ptr<Input::InputManager> InputManager = std::make_shared<Input::InputManager>();
		//std::shared_ptr<Audio::AudioManager> AudioManager = std::make_shared<Audio::AudioManager>();
		std::shared_ptr<UI::UIManager> UIManager = std::make_shared<UI::UIManager>(this, ECSWorld, InputManager);

		// Subsystems
		m_audioManager = RegisterSubsystem<Audio::AudioManager>();

		InputManager->Init(m_window, ECSWorld);

		// Systems
		std::shared_ptr<Rendering::VulkanEngine> vulkanEngine = ECSWorld->RegisterSystem<Rendering::VulkanEngine>();
		std::shared_ptr<Physics::Box2DPhysicsSystem> physicsSystem = ECSWorld->RegisterSystem<Physics::Box2DPhysicsSystem>();
		std::shared_ptr<Scripting::AngelScriptSystem> scriptingSystem = ECSWorld->RegisterSystem<Scripting::AngelScriptSystem>();

		scriptingSystem->SetAudioManager(m_audioManager);

		// Register Components to ECS World
		ECSWorld->RegisterComponent<TransformComponent>();
		ECSWorld->RegisterComponent<Rendering::MeshComponent>();
		ECSWorld->RegisterComponent<Rendering::LightComponent>();
		ECSWorld->RegisterComponent<Physics::Box2DRigidbodyComponent>();
		ECSWorld->RegisterComponent<Physics::Box2DBoxComponent>();
		ECSWorld->RegisterComponent<Physics::Box2DCircleComponent>();
		ECSWorld->RegisterComponent<Scripting::AngelScriptComponent>();

		// Setup Entity Signatures
		ECS::Signature meshSignature;
		meshSignature.set(ECSWorld->GetComponentType<TransformComponent>());
		meshSignature.set(ECSWorld->GetComponentType<Rendering::MeshComponent>());
		ECSWorld->SetSystemSignature<Rendering::VulkanEngine>("Mesh", meshSignature);

		ECS::Signature lightSignature;
		lightSignature.set(ECSWorld->GetComponentType<TransformComponent>());
		lightSignature.set(ECSWorld->GetComponentType<Rendering::LightComponent>());
		ECSWorld->SetSystemSignature<Rendering::VulkanEngine>("Light", lightSignature);

		ECS::Signature scriptSignature;
		scriptSignature.set(ECSWorld->GetComponentType<Scripting::AngelScriptComponent>());
		ECSWorld->SetSystemSignature<Scripting::AngelScriptSystem>("Script", scriptSignature);

		// Register Entity Flags

		// Register Component Flags
		ECSWorld->RegisterComponentFlag<FlagDirty>(true);
		ECSWorld->RegisterComponentFlag<FlagDeleted>();

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
		m_sceneData = std::make_shared<IO::SceneData>(ECSWorld, defaultScenePath);

		// Register Components to Scene Data
		m_sceneData->RegisterComponent<TransformComponent>("Transforms");
		m_sceneData->RegisterComponent<Rendering::MeshComponent>("Meshes");
		m_sceneData->RegisterComponent<Rendering::LightComponent>("Lights");
		m_sceneData->RegisterComponent<Physics::Box2DRigidbodyComponent>("Rigidbodies");
		m_sceneData->RegisterComponent<Physics::Box2DBoxComponent>("Boxes");
		m_sceneData->RegisterComponent<Physics::Box2DCircleComponent>("Circles");
		m_sceneData->RegisterComponent<Scripting::AngelScriptComponent>("Scripts");

		// Load/Initialize Assets
		//AddDefaultAssets();
		Assets::AssetRegistry::Get()->LoadAssetCache();

		// Create Default Scene in code -- used when scene serialization is changed
		//DefaultScene(ECSWorld);
		//PhysicsScene(ECSWorld);
		
		// Load Scene -- normal behaviour
		m_sceneData->LoadAndInit();

		running = true;
		playState = PlayState::STOPPED;

		// Initialize Systems
		vulkanEngine->Init(m_window, UIManager, InputManager);

		for (auto system : ECSWorld->GetAllSystems())
		{
			system->Init();
			system->PreStart();
		}
		
		// Run Game Loop;
		auto lastTime = std::chrono::high_resolution_clock::now(); // Time Count Started
		auto currentTime = std::chrono::high_resolution_clock::now();
		auto accumulatedTime = 0.0;
		auto timeStep = 1 / 60.0; // How often deterministic code like physics should occur
		auto maxTimeStep = 1 / 30.0;

		while (running)
		{
			lastTime = currentTime;
			currentTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> duration = currentTime - lastTime;
			double deltaTime = duration.count();

			// Make sure delta time never exceeds 1/30th of a second to stop
			if (deltaTime > maxTimeStep)
			{
				deltaTime = maxTimeStep;
			}

			// Set delta time for all systems
			for (auto system : ECSWorld->GetAllSystems())
			{
				system->SetDeltaTime(deltaTime);
				system->SetFixedTime(timeStep);
			}

			// Call system start functions to prepare for gameplay
			if (playState == PlayState::STARTED)
			{
				for (auto system : ECSWorld->GetAllSystems())
				{
					system->Start();
				}

				// Get Snapshot of current scene data
				m_sceneData->UpdateData();
				
				accumulatedTime = 0.0;
				playState = PlayState::PLAYING;
			}

			if (playState == PlayState::JUST_PAUSED)
			{
				m_audioManager->PauseAllSounds();

				playState = PlayState::PAUSED;
			}

			if (playState == PlayState::JUST_UNPAUSED)
			{
				m_audioManager->PlayAllSounds();

				playState = PlayState::PLAYING;
			}

			// Fixed Update
			if (playState == PlayState::PLAYING)
			{
				// Add onto accumulated time
				accumulatedTime += deltaTime;
				
				// Perform system updates until simulation is caught up
				std::vector<std::shared_ptr<ECS::System>> fixedUpdateSystems;
				ECSWorld->GetSystemsWithUpdateOrder(ECS::UpdateOrder::FixedUpdate, fixedUpdateSystems);
				while(accumulatedTime >= timeStep)
				{
					accumulatedTime -= timeStep;

					// FixedUpdate Systems
					for (auto system : fixedUpdateSystems)
					{
						system->Update();
					}
				}
			}

			// Input
			InputManager->UpdateInput();

			// Update
			if (playState == PlayState::PLAYING)
			{
				std::vector<std::shared_ptr<ECS::System>> updateSystems;
				ECSWorld->GetSystemsWithUpdateOrder(ECS::UpdateOrder::Update, updateSystems);
				for (auto system : updateSystems)
				{
					system->Update();
				}
			}

			// UI
			UIManager->Update();

			// Rendering
			std::vector<std::shared_ptr<ECS::System>> renderingSystems;
			ECSWorld->GetSystemsWithUpdateOrder(ECS::UpdateOrder::Rendering, renderingSystems);

			for (auto system : renderingSystems)
			{
				system->Update();
			}

			if (playState == PlayState::JUST_STOPPED)
			{
				// Cleanup Systems and ECS
				for (auto system : ECSWorld->GetAllSystems())
				{
					system->Stop();
				}
				ECSWorld->Reset();

				// Re-Initialize Systems and ECS
				m_sceneData->Init();
				vulkanEngine->Start();

				// Perform Pre-Gameplay Initiualization on Systems
				for (auto system : ECSWorld->GetAllSystems())
				{
					system->PreStart();
				}

				m_audioManager->StopAllSounds();

				playState = PlayState::STOPPED;
			}

			// Audio
			m_audioManager->Update();

			if (glfwWindowShouldClose(m_window))
			{
				running = false;
			}

			// Delete All Marked Objects
			ECSWorld->Update();
		}

		// Cleanup All Systems
		for (auto system : ECSWorld->GetAllSystems())
		{
			system->Cleanup();
		}

		UIManager->Cleanup();

		Assets::AssetRegistry::Clear();

		//physicsSystem = nullptr;
		vulkanEngine = nullptr;

		m_audioManager = nullptr;

		ECSWorld = nullptr;

		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	bool Engine::Update()
	{
		

		return true;
	}

	void Engine::Destroy()
	{

	}

	void Engine::AddDefaultAssets()
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

		entities[3]->AddComponent<Rendering::LightComponent>();
		entities[6]->AddComponent<Rendering::LightComponent>();

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

		entities[3]->GetComponent<Rendering::LightComponent>().direction = glm::vec3(1.0f, -1.0f, 0.0f);
		entities[3]->GetComponent<Rendering::LightComponent>().ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
		entities[3]->GetComponent<Rendering::LightComponent>().diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
		entities[3]->GetComponent<Rendering::LightComponent>().innerCutoffAngle = 12.5f;
		entities[3]->GetComponent<Rendering::LightComponent>().outerCutoffAngle = 17.5f;
		entities[3]->GetComponent<Rendering::LightComponent>().constantAttenuation = 1.0f;
		entities[3]->GetComponent<Rendering::LightComponent>().linearAttenuation = 0.09f;
		entities[3]->GetComponent<Rendering::LightComponent>().quadraticAttenuation = 0.032f;
		entities[3]->GetComponent<Rendering::LightComponent>().specularStrength = 0.5f;
		entities[3]->GetComponent<Rendering::LightComponent>().shininess = 16;
		entities[3]->GetComponent<Rendering::LightComponent>().type = Rendering::LightType::SPOT;
		entities[3]->GetComponent<Rendering::LightComponent>().bFlagCastShadows = true;

		entities[6]->GetComponent<Rendering::LightComponent>().direction = glm::vec3(-1.0f, -1.0f, 0.0f);
		entities[6]->GetComponent<Rendering::LightComponent>().ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
		entities[6]->GetComponent<Rendering::LightComponent>().diffuseColor = glm::vec3(0.25f, 0.25f, 1.0f);
		entities[6]->GetComponent<Rendering::LightComponent>().innerCutoffAngle = 12.5f;
		entities[6]->GetComponent<Rendering::LightComponent>().outerCutoffAngle = 17.5f;
		entities[6]->GetComponent<Rendering::LightComponent>().constantAttenuation = 1.0f;
		entities[6]->GetComponent<Rendering::LightComponent>().linearAttenuation = 0.09f;
		entities[6]->GetComponent<Rendering::LightComponent>().quadraticAttenuation = 0.032f;
		entities[6]->GetComponent<Rendering::LightComponent>().specularStrength = 0.5f;
		entities[6]->GetComponent<Rendering::LightComponent>().shininess = 16;
		entities[6]->GetComponent<Rendering::LightComponent>().type = Rendering::LightType::SPOT;
		entities[6]->GetComponent<Rendering::LightComponent>().bFlagCastShadows = false;

		Scripting::AngelScriptComponent script;
		script.name = "ExampleScript";
		script.dir = contentRootPath / "scripts\\Example.pscript";
		entities[0]->AddComponent(script);
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

		lightEntity->AddComponent<TransformComponent>();
		lightEntity->AddComponent<Rendering::LightComponent>();

		lightEntity->GetComponent<TransformComponent>() = { Vector3f(0.0f, 10.0f, 0.0f), Vector3f(0.0f), Vector3f(1.0f) };

		auto& lightComp = lightEntity->GetComponent<Rendering::LightComponent>();
		lightComp.direction = glm::vec3(1.0f, -1.0f, 0.0f);
		lightComp.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
		lightComp.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
		lightComp.innerCutoffAngle = 12.5f;
		lightComp.outerCutoffAngle = 17.5f;
		lightComp.constantAttenuation = 1.0f;
		lightComp.linearAttenuation = 0.09f;
		lightComp.quadraticAttenuation = 0.032f;
		lightComp.specularStrength = 0.5f;
		lightComp.shininess = 16;
		lightComp.type = Rendering::LightType::DIRECTIONAL;
		lightComp.bFlagCastShadows = false;

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