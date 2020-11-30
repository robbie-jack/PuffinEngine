#include "Engine.h"

#include "ECS.h"

#include "TransformComponent.h"
#include "VulkanRenderer.h"
#include "BulletPhysicsSystem.h"
#include "SerializeScene.h"

#include <chrono>

namespace Puffin
{
	void Engine::MainLoop()
	{
		// Managers
		UI::UIManager UIManager;
		Input::InputManager InputManager;
		ECS::World ECSWorld;

		ECSWorld.Init();

		UIManager.SetEngine(this);
		UIManager.SetWorld(&ECSWorld);

		// Systems
		std::shared_ptr<Physics::BulletPhysicsSystem> physicsSystem = ECSWorld.RegisterSystem<Physics::BulletPhysicsSystem>();
		std::shared_ptr<Rendering::VulkanRenderer> renderSystem = ECSWorld.RegisterSystem<Rendering::VulkanRenderer>();

		ECSWorld.RegisterComponent<TransformComponent>();
		ECSWorld.RegisterComponent<Rendering::MeshComponent>();
		ECSWorld.RegisterComponent<Physics::RigidbodyComponent>();

		ECS::Signature renderSignature;
		renderSignature.set(ECSWorld.GetComponentType<TransformComponent>());
		renderSignature.set(ECSWorld.GetComponentType<Rendering::MeshComponent>());
		ECSWorld.SetSystemSignature<Rendering::VulkanRenderer>(renderSignature);

		ECS::Signature physicsSignature;
		physicsSignature.set(ECSWorld.GetComponentType<TransformComponent>());
		physicsSignature.set(ECSWorld.GetComponentType<Physics::RigidbodyComponent>());
		ECSWorld.SetSystemSignature<Physics::BulletPhysicsSystem>(physicsSignature);

		//DefaultScene(&ECSWorld);
		IO::LoadScene("default.scn", &ECSWorld);

		running = true;
		playState = PlayState::STOPPED;

		renderSystem->Init(&UIManager, &InputManager);
		physicsSystem->Start();

		auto lastTime = std::chrono::high_resolution_clock::now(); // Time Count Started
		auto currentTime = std::chrono::high_resolution_clock::now();

		while (running)
		{
			lastTime = currentTime;
			currentTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> duration = currentTime - lastTime;
			float delta_time = duration.count();

			running = renderSystem->Update(&UIManager, &InputManager, delta_time);

			if (playState == PlayState::PLAYING)
			{
				physicsSystem->Update(delta_time);
			}

			ECSWorld.Update();
		}

		physicsSystem->Stop();

		physicsSystem.reset();
		renderSystem.reset();

		UIManager.Cleanup();
		ECSWorld.Cleanup();
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

		world->AddComponent<Physics::RigidbodyComponent>(3);
		world->AddComponent<Physics::RigidbodyComponent>(5);

		// Initialise Components with default values
		world->GetComponent<TransformComponent>(1) = { false, false, Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(2) = { false, false, Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(3) = { false, false, Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		world->GetComponent<TransformComponent>(4) = { false, false, Vector3(-2.0f, 0.0f, 2.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.25f) };
		world->GetComponent<TransformComponent>(5) = { false, false, Vector3(-1.75f, -5.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };

		world->GetComponent<Rendering::MeshComponent>(1).model_path = "models\\chalet.obj";
		world->GetComponent<Rendering::MeshComponent>(1).texture_path = "textures\\chalet.jpg";

		world->GetComponent<Rendering::MeshComponent>(2).model_path = "models\\space_engineer.obj";
		world->GetComponent<Rendering::MeshComponent>(2).texture_path = "textures\\space_engineer.jpg";

		world->GetComponent<Rendering::MeshComponent>(3).model_path = "models\\cube.obj";
		world->GetComponent<Rendering::MeshComponent>(3).texture_path = "textures\\cube.png";
		world->GetComponent<Rendering::MeshComponent>(4).model_path = "models\\cube.obj";
		world->GetComponent<Rendering::MeshComponent>(4).texture_path = "textures\\cube.png";
		world->GetComponent<Rendering::MeshComponent>(5).model_path = "models\\cube.obj";
		world->GetComponent<Rendering::MeshComponent>(5).texture_path = "textures\\cube.png";

		world->GetComponent<Physics::RigidbodyComponent>(3).size = btVector3(1.0f, 1.0f, 1.0f);
		world->GetComponent<Physics::RigidbodyComponent>(3).mass = 1.0f;

		world->GetComponent<Physics::RigidbodyComponent>(5).size = btVector3(1.0f, 1.0f, 1.0f);
		world->GetComponent<Physics::RigidbodyComponent>(5).mass = 0.0f;
	}

	void Engine::Stop()
	{
		if (playState == PlayState::PLAYING | playState == PlayState::PAUSED)
		{
			playState = PlayState::STOPPED;
		}
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