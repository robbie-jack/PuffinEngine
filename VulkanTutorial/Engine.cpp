#include "Engine.h"

#include "ECS.h"

#include "TransformComponent.h"
#include "VulkanRenderer.h"
#include "ReactPhysicsSystem.h"
#include "BulletPhysicsSystem.h"

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

		//UIManager.SetEngine(this);
		UIManager.SetWorld(&ECSWorld);

		// Systems
		std::shared_ptr<Physics::BulletPhysicsSystem> physicsSystem = ECSWorld.RegisterSystem<Physics::BulletPhysicsSystem>();
		std::shared_ptr<Rendering::VulkanRenderer> renderSystem = ECSWorld.RegisterSystem<Rendering::VulkanRenderer>();

		ECSWorld.RegisterComponent<TransformComponent>();
		ECSWorld.RegisterComponent<Rendering::MeshComponent>();

		ECSWorld.RegisterComponent<Physics::BulletPhysicsComponent>();

		ECS::Signature renderSignature;
		renderSignature.set(ECSWorld.GetComponentType<TransformComponent>());
		renderSignature.set(ECSWorld.GetComponentType<Rendering::MeshComponent>());
		ECSWorld.SetSystemSignature<Rendering::VulkanRenderer>(renderSignature);

		ECS::Signature physicsSignature;
		physicsSignature.set(ECSWorld.GetComponentType<TransformComponent>());
		physicsSignature.set(ECSWorld.GetComponentType<Physics::BulletPhysicsComponent>());
		ECSWorld.SetSystemSignature<Physics::BulletPhysicsSystem>(physicsSignature);

		for (int i = 0; i < 5; i++)
		{
			ECS::Entity entity = ECSWorld.CreateEntity();
			ECSWorld.AddComponent<TransformComponent>(entity);
			ECSWorld.AddComponent<Rendering::MeshComponent>(entity);
		}

		ECSWorld.GetComponent<TransformComponent>(1) = { false, false, false, Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		ECSWorld.GetComponent<TransformComponent>(2) = { false, false, false, Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		ECSWorld.GetComponent<TransformComponent>(3) = { false, false, false, Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };
		ECSWorld.GetComponent<TransformComponent>(4) = { false, false, false, Vector3(-2.0f, 0.0f, 2.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.25f) };
		ECSWorld.GetComponent<TransformComponent>(5) = { false, false, false, Vector3(-1.75f, -5.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f) };


		ECSWorld.AddComponent<Physics::BulletPhysicsComponent>(3);
		ECSWorld.AddComponent<Physics::BulletPhysicsComponent>(5);

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
			physicsSystem->Update(delta_time);

			ECSWorld.Update();
		}

		physicsSystem->Stop();

		physicsSystem.reset();
		renderSystem.reset();

		UIManager.Cleanup();
		ECSWorld.Cleanup();
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