#include "Engine.h"

#include <ECS/ECS.h>

#include <Rendering/VulkanEngine.h>
#include <Physics/PhysicsSystem2D.h>
#include <Scripting/AngelScriptSystem.h>
//#include <Physics/BulletPhysicsSystem.h>

#include <Components/AngelScriptComponent.h>
#include <Components/TransformComponent.h>

#include <Input/InputManager.h>
#include <JobManager.h>

#include <SerializeScene.h>
#include <UI/UIManager.h>

#include <chrono>

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
		//Job::JobManager JobManager;

		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		GLFWmonitor* monitor = nullptr;
		GLFWwindow* window = glfwCreateWindow(1280, 720, "Puffin Engine", monitor, NULL);

		ECSWorld->Init();
		InputManager.Init(window, ECSWorld);
		//JobManager.Init();

		// Systems
		//std::shared_ptr<Physics::BulletPhysicsSystem> physicsSystem = ECSWorld->RegisterSystem<Physics::BulletPhysicsSystem>();
		std::shared_ptr<Physics::PhysicsSystem2D> physicsSystem = ECSWorld->RegisterSystem<Physics::PhysicsSystem2D>();
		std::shared_ptr<Rendering::VulkanEngine> vulkanEngine = ECSWorld->RegisterSystem<Rendering::VulkanEngine>();
		std::shared_ptr<Scripting::AngelScriptSystem> scriptingSystem = ECSWorld->RegisterSystem<Scripting::AngelScriptSystem>();

		ECSWorld->RegisterComponent<TransformComponent>();
		ECSWorld->RegisterComponent<Rendering::MeshComponent>();
		ECSWorld->RegisterComponent<Rendering::LightComponent>();
		ECSWorld->RegisterComponent<Physics::RigidbodyComponent2D>();
		ECSWorld->RegisterComponent<Physics::CircleComponent2D>();
		ECSWorld->RegisterComponent<Physics::BoxComponent2D>();
		ECSWorld->RegisterComponent<Scripting::AngelScriptComponent>();

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

		sceneData.scene_name = "content/scenes/physics.pscn";
		IO::LoadSettings("settings.xml", settings);

		// Create Default Scene in code -- used when scene serialization is changed
		//DefaultScene(ECSWorld);
		PhysicsScene(ECSWorld);
		
		// Load Scene -- normal behaviour
		//IO::LoadScene(ECSWorld, sceneData);
		//IO::InitScene(ECSWorld, sceneData);

		ECSWorld->InitEntitySystem();

		IO::SaveScene(ECSWorld, sceneData);

		running = true;
		restarted = false;
		playState = PlayState::STOPPED;

		// Initialize Systems
		vulkanEngine->Init(window, &UIManager);

		physicsSystem->Init();

		scriptingSystem->Init();
		scriptingSystem->Start();
		
		auto lastTime = std::chrono::high_resolution_clock::now(); // Time Count Started
		auto currentTime = std::chrono::high_resolution_clock::now();

		while (running)
		{
			lastTime = currentTime;
			currentTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> duration = currentTime - lastTime;
			float delta_time = duration.count();

			// Input
			InputManager.UpdateInput();

			// Physics/Scripting
			if (playState == PlayState::PLAYING)
			{
				scriptingSystem->Update(delta_time);
				physicsSystem->Update(delta_time);
			}
			else
			{
				scriptingSystem->Update(0.0f);
				physicsSystem->Update(0.0f);
			}

			// UI
			UIManager.Update();

			// Rendering
			vulkanEngine->Update(&UIManager, &InputManager, delta_time);

			if (playState == PlayState::STOPPED && restarted)
			{
				// Cleanup Systems and ECS
				vulkanEngine->StopScene();
				scriptingSystem->Stop();
				physicsSystem->Cleanup();
				ECSWorld->Reset();

				// Re-Initialize Systems and ECS
				IO::LoadScene(ECSWorld, sceneData);
				IO::InitScene(ECSWorld, sceneData);
				vulkanEngine->StartScene();
				scriptingSystem->Start();
				physicsSystem->Init();

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

		//world->AddComponent<Physics::RigidbodyComponent>(3);
		//world->AddComponent<Physics::RigidbodyComponent>(5);

		// Initialize Components with default values
		world->GetComponent<TransformComponent>(1) = { Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(2) = { Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(3) = { Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(4) = { Vector3(-10.0f, 0.0f, 2.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.25f) };
		world->GetComponent<TransformComponent>(5) = { Vector3(-1.75f, -5.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(6) = { Vector3(0.0f, -10.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 1.0f, 10.0f) };
		world->GetComponent<TransformComponent>(7) = { Vector3(5.0f, 0.0f, 2.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.25f) };

		world->GetComponent<Rendering::MeshComponent>(1).model_path = "content\\models\\chalet.psm";
		world->GetComponent<Rendering::MeshComponent>(1).texture_path = "content\\textures\\chalet.jpg";

		world->GetComponent<Rendering::MeshComponent>(2).model_path = "content\\models\\sphere.psm";
		world->GetComponent<Rendering::MeshComponent>(2).texture_path = "content\\textures\\cube.png";

		world->GetComponent<Rendering::MeshComponent>(3).model_path = "content\\models\\cube.psm";
		world->GetComponent<Rendering::MeshComponent>(3).texture_path = "content\\textures\\cube.png";
		world->GetComponent<Rendering::MeshComponent>(4).model_path = "content\\models\\cube.psm";
		world->GetComponent<Rendering::MeshComponent>(4).texture_path = "content\\textures\\cube.png";
		world->GetComponent<Rendering::MeshComponent>(5).model_path = "content\\models\\cube.psm";
		world->GetComponent<Rendering::MeshComponent>(5).texture_path = "content\\textures\\cube.png";
		world->GetComponent<Rendering::MeshComponent>(6).model_path = "content\\models\\cube.psm";
		world->GetComponent<Rendering::MeshComponent>(6).texture_path = "content\\textures\\cube.png";
		world->GetComponent<Rendering::MeshComponent>(7).model_path = "content\\models\\cube.psm";
		world->GetComponent<Rendering::MeshComponent>(7).texture_path = "content\\textures\\cube.png";

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

		/*world->GetComponent<Physics::RigidbodyComponent>(3).size = btVector3(1.0f, 1.0f, 1.0f);
		world->GetComponent<Physics::RigidbodyComponent>(3).mass = 1.0f;

		world->GetComponent<Physics::RigidbodyComponent>(5).size = btVector3(1.0f, 1.0f, 1.0f);
		world->GetComponent<Physics::RigidbodyComponent>(5).mass = 0.0f;*/

		Scripting::AngelScriptComponent script;
		script.name = "Game";
		script.dir = "C:/Projects/PuffinProject/content/scripts/game.as";
		world->AddComponent(1, script);
	}

	void Engine::PhysicsScene(std::shared_ptr<ECS::World> world)
	{
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
		world->AddComponent<Physics::CircleComponent2D>(boxEntity);

		world->GetComponent<TransformComponent>(boxEntity) = { Vector3(0.0f, 10.0f, 0.0f), Vector3(0.0f), Vector3(1.0f) };

		world->GetComponent<Rendering::MeshComponent>(boxEntity).model_path = "content\\models\\cube.psm";
		world->GetComponent<Rendering::MeshComponent>(boxEntity).texture_path = "content\\textures\\cube.png";

		world->GetComponent<Physics::RigidbodyComponent2D>(boxEntity).invMass = 1.0f;
		world->GetComponent<Physics::RigidbodyComponent2D>(boxEntity).elasticity = 0.75f;

		/*world->GetComponent<Physics::ShapeComponent2D>(boxEntity).type = Physics::Collision2D::ShapeType::BOX;
		world->GetComponent<Physics::ShapeComponent2D>(boxEntity).box.halfExtent = Vector2(1.0f);*/

		world->GetComponent<Physics::CircleComponent2D>(boxEntity).radius = 1.0f;

		// Create Floor Entity
		ECS::Entity floorEntity = world->CreateEntity();

		world->SetEntityName(floorEntity, "Floor");

		world->AddComponent<TransformComponent>(floorEntity);
		world->AddComponent<Rendering::MeshComponent>(floorEntity);
		world->AddComponent<Physics::RigidbodyComponent2D>(floorEntity);
		world->AddComponent<Physics::CircleComponent2D>(floorEntity);

		world->GetComponent<TransformComponent>(floorEntity) = { Vector3(0.0f), Vector3(0.0f), Vector3(10.0f, 1.0f, 1.0f) };

		world->GetComponent<Rendering::MeshComponent>(floorEntity).model_path = "content\\models\\cube.psm";
		world->GetComponent<Rendering::MeshComponent>(floorEntity).texture_path = "content\\textures\\cube.png";

		world->GetComponent<Physics::RigidbodyComponent2D>(floorEntity).invMass = 0.0f; // Setting mass to zero makes rigidbody kinematic instead of dynamic

		/*world->GetComponent<Physics::ShapeComponent2D>(floorEntity).type = Physics::Collision2D::ShapeType::BOX;
		world->GetComponent<Physics::ShapeComponent2D>(floorEntity).box.halfExtent = Vector2(10.0f, 1.0f);*/

		world->GetComponent<Physics::CircleComponent2D>(floorEntity).radius = 1.0f;
	}

	void Engine::Play()
	{
		switch (playState)
		{
		case PlayState::STOPPED:
			playState = PlayState::PLAYING;
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