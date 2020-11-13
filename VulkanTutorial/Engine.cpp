#include "Engine.h"

#include "ECS.h"

#include "TransformSystem.h"
#include "VulkanRenderer.h"
#include "ReactPhysicsSystem.h"

#include <chrono>

namespace Puffin
{
	void Engine::MainLoop()
	{
		// Managers
		UI::UIManager UIManager;
		Input::InputManager InputManager;
		//ECS::EntityManager EntityManager;
		//ECS::ComponentManager ComponentManager;
		//ECS::SystemManager SystemManager;
		ECS::World ECSWorld;

		ECSWorld.Init();

		UIManager.SetWorld(&ECSWorld);

		// Systems
		Physics::ReactPhysicsSystem physicsSystem;
		std::shared_ptr<Rendering::VulkanRenderer> renderSystem = ECSWorld.RegisterSystem<Rendering::VulkanRenderer>();

		ECSWorld.RegisterComponent<TransformComponent>();
		ECSWorld.RegisterComponent<Rendering::MeshComponent>();

		ECS::Signature renderSignature;
		renderSignature.set(ECSWorld.GetComponentType<TransformComponent>());
		renderSignature.set(ECSWorld.GetComponentType<Rendering::MeshComponent>());
		ECSWorld.SetSystemSignature<Rendering::VulkanRenderer>(renderSignature);

		for (int i = 0; i < 5; i++)
		{
			ECS::Entity entity = ECSWorld.CreateEntity();
			ECSWorld.AddComponent<TransformComponent>(entity);
			//Rendering::MeshComponent comp;
			//ECSWorld.AddComponent<Rendering::MeshComponent>(entity, comp);
			ECSWorld.AddComponent<Rendering::MeshComponent>(entity);
			renderSystem->AddComponent();
		}

		//entitySystem.GetEntity(3)->AttachComponent(physicsSystem.AddComponent());
		//entitySystem.GetEntity(5)->AttachComponent(physicsSystem.AddComponent());
		physicsSystem.AddComponent();
		physicsSystem.AddComponent();

		running = true;
		playState = PlayState::STOPPED;

		/*for (int i = 0; i < systems.size(); i++)
		{
			systems[i]->Init();
		}*/

		//entitySystem.Init();
		//transformSystem.Init();
		physicsSystem.Init();
		renderSystem->Init(&UIManager, &InputManager);

		//entitySystem.Start();
		//transformSystem.Start();
		physicsSystem.Start();
		//renderSystem->Start();

		auto lastTime = std::chrono::high_resolution_clock::now(); // Time Count Started
		auto currentTime = std::chrono::high_resolution_clock::now();

		while (running)
		{
			lastTime = currentTime;
			currentTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> duration = currentTime - lastTime;
			float delta_time = duration.count();

			//entitySystem.Update(delta_time);
			//transformSystem.Update(delta_time);
			//physicsSystem.Update(delta_time);
			running = renderSystem->Update(&UIManager, &InputManager, delta_time);

			//Update(delta_time);
		}

		UIManager.Cleanup();
	}

	void Engine::Update(float dt)
	{
		// Only Update Systems marked updateWhenPlaying when Play State = Playing
		for (int i = 0; i < systems.size(); i++)
		{
			if (systems[i]->GetUpdateWhenPlaying() == false)
			{
				if (systems[i]->Update(dt) == false)
				{
					running = false;
				}
			}
			else
			{
				if (playState == PlayState::PLAYING)
				{
					if (systems[i]->Update(dt) == false)
					{
						running = false;
					}
				}
			}
		}
	}

	void Engine::AddSystem(System* sys)
	{
		systems.push_back(sys);
	}

	void Engine::Start()
	{
		/*for (int i = 0; i < systems.size(); i++)
		{
			systems[i]->Start();
		}*/

		playState = PlayState::PLAYING;
	}

	void Engine::Stop()
	{
		if (playState == PlayState::PLAYING | playState == PlayState::PAUSED)
		{
			/*for (int i = 0; i < systems.size(); i++)
			{
				systems[i]->Stop();
			}*/

			playState = PlayState::STOPPED;
		}
	}

	void Engine::Play()
	{
		switch (playState)
		{
		case PlayState::STOPPED:
			Start();
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