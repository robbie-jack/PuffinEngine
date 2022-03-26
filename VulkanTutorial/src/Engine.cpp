#include "Engine.h"

#include <ECS/ECS.h>

#include <Rendering/VulkanEngine.h>
#include <Physics/PhysicsSystem2D.h>
#include <Scripting/AngelScriptSystem.h>

#include <Components/AngelScriptComponent.h>
#include <Components/TransformComponent.h>

#include "Types/ComponentFlags.h"

#include <Input/InputManager.h>

#include <SerializeScene.h>
#include <UI/UIManager.h>

#include "Assets/AssetRegistry.h"
#include "Assets/MeshAsset.h"
#include "Assets/TextureAsset.h"

#include <chrono>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace Puffin
{
	const int WIDTH = 1280;
	const int HEIGHT = 720;

	void Engine::MainLoop()
	{
		// Managers/ECS World
		std::shared_ptr<ECS::World> ECSWorld = std::make_shared<ECS::World>();
		UI::UIManager UIManager(this, ECSWorld);
		Input::InputManager InputManager;

		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		GLFWmonitor* monitor = nullptr;
		GLFWwindow* window = glfwCreateWindow(1280, 720, "Puffin Engine", monitor, NULL);

		ECSWorld->Init();
		InputManager.Init(window, ECSWorld);

		// Systems
		std::shared_ptr<Rendering::VulkanEngine> vulkanEngine = ECSWorld->RegisterSystem<Rendering::VulkanEngine>();
		std::shared_ptr<Physics::PhysicsSystem2D> physicsSystem = ECSWorld->RegisterSystem<Physics::PhysicsSystem2D>();
		std::shared_ptr<Scripting::AngelScriptSystem> scriptingSystem = ECSWorld->RegisterSystem<Scripting::AngelScriptSystem>();

		// Register Components
		ECSWorld->RegisterComponent<TransformComponent>();
		ECSWorld->RegisterComponent<Rendering::MeshComponent>();
		ECSWorld->RegisterComponent<Rendering::LightComponent>();
		ECSWorld->RegisterComponent<Physics::RigidbodyComponent2D>();
		ECSWorld->RegisterComponent<Physics::CircleComponent2D>();
		ECSWorld->RegisterComponent<Physics::BoxComponent2D>();
		ECSWorld->RegisterComponent<Scripting::AngelScriptComponent>();

		// Register Component Flags
		ECSWorld->RegisterComponentFlag<FlagDirty>(true);
		ECSWorld->RegisterComponentFlag<FlagDeleted>();

		// Setup Entity Signatures
		ECS::Signature meshSignature;
		meshSignature.set(ECSWorld->GetComponentType<TransformComponent>());
		meshSignature.set(ECSWorld->GetComponentType<Rendering::MeshComponent>());
		ECSWorld->SetSystemSignature<Rendering::VulkanEngine>("Mesh", meshSignature);

		ECS::Signature lightSignature;
		lightSignature.set(ECSWorld->GetComponentType<TransformComponent>());
		lightSignature.set(ECSWorld->GetComponentType<Rendering::LightComponent>());
		ECSWorld->SetSystemSignature<Rendering::VulkanEngine>("Light", lightSignature);

		ECS::Signature rigidbodySignature;
		rigidbodySignature.set(ECSWorld->GetComponentType<TransformComponent>());
		rigidbodySignature.set(ECSWorld->GetComponentType<Physics::RigidbodyComponent2D>());
		ECSWorld->SetSystemSignature<Physics::PhysicsSystem2D>("Rigidbody", rigidbodySignature);

		ECS::Signature circleSignature;
		circleSignature.set(ECSWorld->GetComponentType<TransformComponent>());
		circleSignature.set(ECSWorld->GetComponentType<Physics::CircleComponent2D>());
		ECSWorld->SetSystemSignature<Physics::PhysicsSystem2D>("CircleCollision", circleSignature);

		ECS::Signature boxSignature;
		boxSignature.set(ECSWorld->GetComponentType<TransformComponent>());
		boxSignature.set(ECSWorld->GetComponentType<Physics::BoxComponent2D>());
		ECSWorld->SetSystemSignature<Physics::PhysicsSystem2D>("BoxCollision", boxSignature);

		ECS::Signature scriptSignature;
		scriptSignature.set(ECSWorld->GetComponentType<Scripting::AngelScriptComponent>());
		ECSWorld->SetSystemSignature<Scripting::AngelScriptSystem>("Script", scriptSignature);

		// Load Project File
		fs::path projectPath = fs::path("C:\\Projects\\PuffinProject\\Puffin.pproject");
		fs::path projectDirPath = projectPath;
		projectDirPath.remove_filename();

		IO::LoadProject(projectPath, projectFile);

		// Register Assets
		Assets::AssetRegistry::Get()->RegisterAssetType<Assets::StaticMeshAsset>();
		Assets::AssetRegistry::Get()->RegisterAssetType<Assets::TextureAsset>();

		// Load Asset Cache
		Assets::AssetRegistry::Get()->ProjectName(projectFile.name);
		Assets::AssetRegistry::Get()->ProjectRoot(projectDirPath);
		//Assets::AssetRegistry::Get()->LoadAssetCache();

		// Load Project Settings
		IO::LoadSettings(projectDirPath.parent_path() / "settings.json", settings);

		// Load Default Scene (if set)
		sceneData.scene_path = projectDirPath.parent_path() / "content" / projectFile.defaultScenePath;

		// Create Default Scene in code -- used when scene serialization is changed
		DefaultScene(ECSWorld);
		//PhysicsScene(ECSWorld);
		
		// Load Scene -- normal behaviour
		//IO::LoadScene(ECSWorld, sceneData);
		//IO::InitScene(ECSWorld, sceneData);

		IO::SaveScene(ECSWorld, sceneData);

		Assets::AssetRegistry::Get()->SaveAssetCache();

		running = true;
		restarted = false;
		playState = PlayState::STOPPED;

		// Initialize Systems
		vulkanEngine->Init(window, &UIManager, &InputManager);

		for (auto system : ECSWorld->GetAllSystems())
		{
			system->Init();
		}
		
		// Run Game Loop;
		auto lastTime = std::chrono::high_resolution_clock::now(); // Time Count Started
		auto currentTime = std::chrono::high_resolution_clock::now();
		auto accumulatedTime = 0.0;
		auto timeStep = 1 / 60.0;

		while (running)
		{
			lastTime = currentTime;
			currentTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> duration = currentTime - lastTime;
			double deltaTime = duration.count();

			// Set delta time for all systems
			for (auto system : ECSWorld->GetAllSystems())
			{
				system->SetDeltaTime(deltaTime);
			}

			// Call system start functions to prepare for gameplay
			if (playState == PlayState::STARTED)
			{
				for (auto system : ECSWorld->GetAllSystems())
				{
					system->Start();
				}
				
				accumulatedTime = 0.0;
				playState = PlayState::PLAYING;
			}

			// Fixed Update
			if (playState == PlayState::PLAYING)
			{
				accumulatedTime += deltaTime;
				
				while(accumulatedTime >= timeStep)
				{
					accumulatedTime -= timeStep;

					// Update Physics System
					physicsSystem->Update();

					// FixedUpdate Systems
					for (auto system : ECSWorld->GetSystemsWithUpdateOrder(ECS::UpdateOrder::FixedUpdate))
					{
						system->Update();
					}
				}
			}

			// Input
			InputManager.UpdateInput();

			// Update
			if (playState == PlayState::PLAYING)
			{
				for (auto system : ECSWorld->GetSystemsWithUpdateOrder(ECS::UpdateOrder::Update))
				{
					system->Update();
				}
			}

			// UI
			UIManager.Update();

			// Rendering
			vulkanEngine->Update();

			if (playState == PlayState::STOPPED && restarted)
			{
				// Cleanup Systems and ECS
				for (auto system : ECSWorld->GetAllSystems())
				{
					system->Stop();
				}
				ECSWorld->Reset();

				// Re-Initialize Systems and ECS
				IO::LoadScene(ECSWorld, sceneData);
				IO::InitScene(ECSWorld, sceneData);
				vulkanEngine->Start();

				restarted = false;
			}

			if (glfwWindowShouldClose(window))
			{
				running = false;
			}

			// Delete All Marked Objects
			ECSWorld->Update();
		}

		scriptingSystem->Stop();
		physicsSystem->Cleanup();
		vulkanEngine->Cleanup();
		UIManager.Cleanup();
		ECSWorld->Cleanup();

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void Engine::DefaultScene(std::shared_ptr<ECS::World> world)
	{
		// Initialize Assets
		fs::path contentRootPath = Assets::AssetRegistry::Get()->ContentRoot();

		const fs::path& meshPath1 = contentRootPath / "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = contentRootPath / "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = contentRootPath / "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = contentRootPath / "meshes\\space_engineer.pstaticmesh";

		UUID meshId1 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath1)->ID();
		UUID meshId2 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath2)->ID();
		UUID meshId3 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath3)->ID();
		UUID meshId4 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath4)->ID();

		const fs::path& texturePath1 = contentRootPath / "textures\\chalet.ptexture";
		const fs::path& texturePath2 = contentRootPath / "textures\\cube.ptexture";

		UUID textureId1 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(texturePath1)->ID();
		UUID textureId2 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(texturePath2)->ID();

		// Initialize EntityManager with Existing Entities
		world->InitEntitySystem();

		// Add Default Scene Components to ECS
		for (int i = 0; i < 7; i++)
		{
			ECS::Entity entity = world->CreateEntity();
			world->AddComponent<TransformComponent>(entity);
			world->AddComponent<Rendering::MeshComponent>(entity);
		}

		world->SetEntityName(1, "House");
		world->SetEntityName(2, "Sphere");
		world->SetEntityName(3, "Falling Cube");
		world->SetEntityName(4, "Light");
		world->SetEntityName(5, "Static Cube");
		world->SetEntityName(6, "Plane");
		world->SetEntityName(7, "Light 2");

		world->AddComponent<Rendering::LightComponent>(4);
		world->AddComponent<Rendering::LightComponent>(7);

		// Initialize Components with default values
		world->GetComponent<TransformComponent>(1) = { Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(2) = { Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(3) = { Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(4) = { Vector3(-10.0f, 0.0f, 2.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.25f) };
		world->GetComponent<TransformComponent>(5) = { Vector3(-1.75f, -5.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(6) = { Vector3(0.0f, -10.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 1.0f, 10.0f) };
		world->GetComponent<TransformComponent>(7) = { Vector3(5.0f, 0.0f, 2.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.25f) };

		world->GetComponent<Rendering::MeshComponent>(1).meshAssetID = meshId1;
		world->GetComponent<Rendering::MeshComponent>(1).textureAssetID = textureId1;

		world->GetComponent<Rendering::MeshComponent>(2).meshAssetID = meshId2;
		world->GetComponent<Rendering::MeshComponent>(2).textureAssetID = textureId2;

		world->GetComponent<Rendering::MeshComponent>(3).meshAssetID = meshId3;
		world->GetComponent<Rendering::MeshComponent>(3).textureAssetID = textureId2;
		world->GetComponent<Rendering::MeshComponent>(4).meshAssetID = meshId3;
		world->GetComponent<Rendering::MeshComponent>(4).textureAssetID = textureId2;
		world->GetComponent<Rendering::MeshComponent>(5).meshAssetID = meshId3;
		world->GetComponent<Rendering::MeshComponent>(5).textureAssetID = textureId2;
		world->GetComponent<Rendering::MeshComponent>(6).meshAssetID = meshId3;
		world->GetComponent<Rendering::MeshComponent>(6).textureAssetID = textureId2;
		world->GetComponent<Rendering::MeshComponent>(7).meshAssetID = meshId3;
		world->GetComponent<Rendering::MeshComponent>(7).textureAssetID = textureId2;

		world->GetComponent<Rendering::LightComponent>(4).direction = glm::vec3(1.0f, -1.0f, 0.0f);
		world->GetComponent<Rendering::LightComponent>(4).ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
		world->GetComponent<Rendering::LightComponent>(4).diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
		world->GetComponent<Rendering::LightComponent>(4).innerCutoffAngle = 12.5f;
		world->GetComponent<Rendering::LightComponent>(4).outerCutoffAngle = 17.5f;
		world->GetComponent<Rendering::LightComponent>(4).constantAttenuation = 1.0f;
		world->GetComponent<Rendering::LightComponent>(4).linearAttenuation = 0.09f;
		world->GetComponent<Rendering::LightComponent>(4).quadraticAttenuation = 0.032f;
		world->GetComponent<Rendering::LightComponent>(4).specularStrength = 0.5f;
		world->GetComponent<Rendering::LightComponent>(4).shininess = 16;
		world->GetComponent<Rendering::LightComponent>(4).type = Rendering::LightType::SPOT;
		world->GetComponent<Rendering::LightComponent>(4).bFlagCastShadows = true;

		world->GetComponent<Rendering::LightComponent>(7).direction = glm::vec3(-1.0f, -1.0f, 0.0f);
		world->GetComponent<Rendering::LightComponent>(7).ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
		world->GetComponent<Rendering::LightComponent>(7).diffuseColor = glm::vec3(0.25f, 0.25f, 1.0f);
		world->GetComponent<Rendering::LightComponent>(7).innerCutoffAngle = 12.5f;
		world->GetComponent<Rendering::LightComponent>(7).outerCutoffAngle = 17.5f;
		world->GetComponent<Rendering::LightComponent>(7).constantAttenuation = 1.0f;
		world->GetComponent<Rendering::LightComponent>(7).linearAttenuation = 0.09f;
		world->GetComponent<Rendering::LightComponent>(7).quadraticAttenuation = 0.032f;
		world->GetComponent<Rendering::LightComponent>(7).specularStrength = 0.5f;
		world->GetComponent<Rendering::LightComponent>(7).shininess = 16;
		world->GetComponent<Rendering::LightComponent>(7).type = Rendering::LightType::SPOT;
		world->GetComponent<Rendering::LightComponent>(7).bFlagCastShadows = false;

		/*Scripting::AngelScriptComponent script;
		script.name = "Game";
		script.dir = GetProjectContentPath() / "scripts\\game.as";
		world->AddComponent(1, script);*/
	}

	void Engine::PhysicsScene(std::shared_ptr<ECS::World> world)
	{
		// Initialize Assets
		fs::path contentRootPath = Assets::AssetRegistry::Get()->ContentRoot();

		const fs::path& meshPath1 = contentRootPath / "meshes\\chalet.pstaticmesh";
		const fs::path& meshPath2 = contentRootPath / "meshes\\sphere.pstaticmesh";
		const fs::path& meshPath3 = contentRootPath / "meshes\\cube.pstaticmesh";
		const fs::path& meshPath4 = contentRootPath / "meshes\\space_engineer.pstaticmesh";

		UUID meshId1 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath1)->ID();
		UUID meshId2 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath2)->ID();
		UUID meshId3 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath3)->ID();
		UUID meshId4 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(meshPath4)->ID();

		const fs::path& texturePath1 = contentRootPath / "textures\\chalet.jpg";
		const fs::path& texturePath2 = contentRootPath / "textures\\cube.png";

		UUID textureId1 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(texturePath1)->ID();
		UUID textureId2 = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(texturePath2)->ID();

		world->InitEntitySystem();

		// Creater Light Entity
		ECS::Entity lightEntity = world->CreateEntity();

		world->SetEntityName(lightEntity, "Light");

		world->AddComponent<TransformComponent>(lightEntity);
		world->AddComponent<Rendering::LightComponent>(lightEntity);

		world->GetComponent<TransformComponent>(lightEntity) = { Vector3(0.0f, 10.0f, 0.0f), Vector3(0.0f), Vector3(1.0f) };

		world->GetComponent<Rendering::LightComponent>(lightEntity).direction = glm::vec3(1.0f, -1.0f, 0.0f);
		world->GetComponent<Rendering::LightComponent>(lightEntity).ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
		world->GetComponent<Rendering::LightComponent>(lightEntity).diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
		world->GetComponent<Rendering::LightComponent>(lightEntity).innerCutoffAngle = 12.5f;
		world->GetComponent<Rendering::LightComponent>(lightEntity).outerCutoffAngle = 17.5f;
		world->GetComponent<Rendering::LightComponent>(lightEntity).constantAttenuation = 1.0f;
		world->GetComponent<Rendering::LightComponent>(lightEntity).linearAttenuation = 0.09f;
		world->GetComponent<Rendering::LightComponent>(lightEntity).quadraticAttenuation = 0.032f;
		world->GetComponent<Rendering::LightComponent>(lightEntity).specularStrength = 0.5f;
		world->GetComponent<Rendering::LightComponent>(lightEntity).shininess = 16;
		world->GetComponent<Rendering::LightComponent>(lightEntity).type = Rendering::LightType::DIRECTIONAL;
		world->GetComponent<Rendering::LightComponent>(lightEntity).bFlagCastShadows = false;

		// Create Box Entity
		ECS::Entity boxEntity = world->CreateEntity();

		world->SetEntityName(boxEntity, "Box");

		world->AddComponent<TransformComponent>(boxEntity);
		world->AddComponent<Rendering::MeshComponent>(boxEntity);
		world->AddComponent<Physics::RigidbodyComponent2D>(boxEntity);
		//world->AddComponent<Physics::BoxComponent2D>(boxEntity);
		world->AddComponent<Physics::CircleComponent2D>(boxEntity);

		world->GetComponent<TransformComponent>(boxEntity) = { Vector3(0.0f, 10.0f, 0.0f), Vector3(0.0f), Vector3(1.0f) };

		world->GetComponent<Rendering::MeshComponent>(boxEntity).meshAssetID = meshId2;
		world->GetComponent<Rendering::MeshComponent>(boxEntity).textureAssetID = textureId2;

		world->GetComponent<Physics::RigidbodyComponent2D>(boxEntity).invMass = 1.0f;
		world->GetComponent<Physics::RigidbodyComponent2D>(boxEntity).elasticity = .5f;

		// Create Floor Entity
		ECS::Entity floorEntity = world->CreateEntity();

		world->SetEntityName(floorEntity, "Floor");

		world->AddComponent<TransformComponent>(floorEntity);
		world->AddComponent<Rendering::MeshComponent>(floorEntity);
		world->AddComponent<Physics::RigidbodyComponent2D>(floorEntity);
		//world->AddComponent<Physics::BoxComponent2D>(floorEntity);
		world->AddComponent<Physics::CircleComponent2D>(floorEntity);

		world->GetComponent<TransformComponent>(floorEntity) = { Vector3(.0f), Vector3(0.0f), Vector3(1.0f, 1.0f, 1.0f) };

		world->GetComponent<Rendering::MeshComponent>(floorEntity).meshAssetID = meshId2;
		world->GetComponent<Rendering::MeshComponent>(floorEntity).textureAssetID = textureId2;

		world->GetComponent<Physics::RigidbodyComponent2D>(floorEntity).invMass = 0.0f; // Setting mass to zero makes rigidbody kinematic instead of dynamic
	}

	void Engine::Play()
	{
		switch (playState)
		{
		case PlayState::STOPPED:
			playState = PlayState::STARTED;
			break;
		case PlayState::PLAYING:
			playState = PlayState::PAUSED;
			break;
		case PlayState::PAUSED:
			playState = PlayState::PLAYING;
			break;
		}
	}

	void Engine::Restart()
	{
		if (playState == PlayState::PLAYING || playState == PlayState::PAUSED)
		{
			playState = PlayState::STOPPED;
			restarted = true;
		}
	}

	void Engine::Exit()
	{
		running = false;
	}
}