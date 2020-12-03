#include "Engine.h"

#include "ECS.h"

#include "TransformComponent.h"
//#include "VulkanRenderer.h"
#include "UIManager.h"
#include "InputManager.h"
#include "VulkanEngine.h"
#include "BulletPhysicsSystem.h"
#include "SerializeScene.h"

#include <chrono>

namespace Puffin
{
	const int WIDTH = 1280;
	const int HEIGHT = 720;

	void Engine::MainLoop()
	{
		// Managers/ECS
		/*ECS::World ECSWorld;
		UI::UIManager UIManager;
		Input::InputManager InputManager;

		ECSWorld.Init();

		UIManager.SetEngine(this);
		UIManager.SetWorld(&ECSWorld);*/

		// Systems
		//std::shared_ptr<Physics::BulletPhysicsSystem> physicsSystem = ECSWorld.RegisterSystem<Physics::BulletPhysicsSystem>();
		//std::shared_ptr<Rendering::VulkanRenderer> renderSystem = ECSWorld.RegisterSystem<Rendering::VulkanRenderer>();
		Rendering::VulkanEngine vulkanEngine;

		/*ECSWorld.RegisterComponent<TransformComponent>();
		ECSWorld.RegisterComponent<Rendering::MeshComponent>();
		ECSWorld.RegisterComponent<Rendering::LightComponent>();
		ECSWorld.RegisterComponent<Physics::RigidbodyComponent>();*/

		/*ECS::Signature meshSignature;
		meshSignature.set(ECSWorld.GetComponentType<TransformComponent>());
		meshSignature.set(ECSWorld.GetComponentType<Rendering::MeshComponent>());
		ECSWorld.SetSystemSignature<Rendering::VulkanRenderer>("Mesh", meshSignature);

		ECS::Signature lightSignature;
		lightSignature.set(ECSWorld.GetComponentType<TransformComponent>());
		lightSignature.set(ECSWorld.GetComponentType<Rendering::LightComponent>());
		ECSWorld.SetSystemSignature<Rendering::VulkanRenderer>("Light", lightSignature);*/

		/*ECS::Signature rigidbodySignature;
		rigidbodySignature.set(ECSWorld.GetComponentType<TransformComponent>());
		rigidbodySignature.set(ECSWorld.GetComponentType<Physics::RigidbodyComponent>());
		ECSWorld.SetSystemSignature<Physics::BulletPhysicsSystem>("Rigidbody", rigidbodySignature);*/

		//DefaultScene(&ECSWorld);

		/*sceneData.scene_name = "assets/scenes/default.scn";
		IO::LoadScene(&ECSWorld, sceneData);
		IO::InitScene(&ECSWorld, sceneData);*/

		running = true;
		restarted = false;
		playState = PlayState::STOPPED;

		// Initialize Systems
		/*GLFWwindow* window = renderSystem->InitWindow();
		renderSystem->InitVulkan(&UIManager);*/
		GLFWwindow* window = vulkanEngine.Init();
		//physicsSystem->Start();

		// Init Input
		//InputManager.SetupInput(window);

		auto lastTime = std::chrono::high_resolution_clock::now(); // Time Count Started
		auto currentTime = std::chrono::high_resolution_clock::now();

		while (running)
		{
			lastTime = currentTime;
			currentTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> duration = currentTime - lastTime;
			float delta_time = duration.count();

			//InputManager.UpdateInput(window);
			//running = renderSystem->Update(&UIManager, &InputManager, delta_time);
			vulkanEngine.Render();

			if (playState == PlayState::PLAYING)
			{
				//physicsSystem->Update(delta_time);
			}

			//ECSWorld.Update();

			//UIManager.Update();

			if (playState == PlayState::STOPPED)
			{
				if (restarted)
				{
					// Cleanup Systems and ECS
					//renderSystem->Stop();
					//physicsSystem->Stop();
					//ECSWorld.Reset();

					// Re-Initialize Systems and ECS
					//IO::LoadScene(&ECSWorld, sceneData);
					//IO::InitScene(&ECSWorld, sceneData);
					//renderSystem->Start();
					//physicsSystem->Start();

					restarted = false;
				}
			}

			running = false;
		}

		//physicsSystem->Stop();

		//physicsSystem.reset();
		//renderSystem.reset();
		vulkanEngine.Cleanup();

		//UIManager.Cleanup();
		//ECSWorld.Cleanup();

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void Engine::DefaultScene(ECS::World* world)
	{
		// Add Default Scene Components to ECS
		for (int i = 0; i < 5; i++)
		{
			ECS::Entity entity = world->CreateEntity();
			world->AddComponent<TransformComponent>(entity);
			world->AddComponent<Rendering::MeshComponent>(entity);
		}

		world->AddComponent<Rendering::LightComponent>(4);

		world->AddComponent<Physics::RigidbodyComponent>(3);
		world->AddComponent<Physics::RigidbodyComponent>(5);

		// Initialise Components with default values
		world->GetComponent<TransformComponent>(1) = { false, false, Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(2) = { false, false, Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(3) = { false, false, Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(4) = { false, false, Vector3(-2.0f, 0.0f, 2.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.25f) };
		world->GetComponent<TransformComponent>(5) = { false, false, Vector3(-1.75f, -5.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };

		world->GetComponent<Rendering::MeshComponent>(1).model_path = "assets\\models\\chalet.asset_m";
		world->GetComponent<Rendering::MeshComponent>(1).texture_path = "textures\\chalet.jpg";

		world->GetComponent<Rendering::MeshComponent>(2).model_path = "assets\\models\\space_engineer.asset_m";
		world->GetComponent<Rendering::MeshComponent>(2).texture_path = "textures\\space_engineer.jpg";

		world->GetComponent<Rendering::MeshComponent>(3).model_path = "assets\\models\\cube.asset_m";
		world->GetComponent<Rendering::MeshComponent>(3).texture_path = "textures\\cube.png";
		world->GetComponent<Rendering::MeshComponent>(4).model_path = "assets\\models\\cube.asset_m";
		world->GetComponent<Rendering::MeshComponent>(4).texture_path = "textures\\cube.png";
		world->GetComponent<Rendering::MeshComponent>(5).model_path = "assets\\models\\cube.asset_m";
		world->GetComponent<Rendering::MeshComponent>(5).texture_path = "textures\\cube.png";

		world->GetComponent<Rendering::LightComponent>(4).uniformBuffer.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
		world->GetComponent<Rendering::LightComponent>(4).uniformBuffer.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
		world->GetComponent<Rendering::LightComponent>(4).uniformBuffer.specularStrength = 0.5f;
		world->GetComponent<Rendering::LightComponent>(4).uniformBuffer.shininess = 16;

		world->GetComponent<Physics::RigidbodyComponent>(3).size = btVector3(1.0f, 1.0f, 1.0f);
		world->GetComponent<Physics::RigidbodyComponent>(3).mass = 1.0f;

		world->GetComponent<Physics::RigidbodyComponent>(5).size = btVector3(1.0f, 1.0f, 1.0f);
		world->GetComponent<Physics::RigidbodyComponent>(5).mass = 0.0f;
	}

	void Engine::Restart()
	{
		if (playState == PlayState::PLAYING | playState == PlayState::PAUSED)
		{
			playState = PlayState::STOPPED;
		}

		restarted = true;
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
}