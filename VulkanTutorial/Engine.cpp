#include "Engine.h"

#include "EntitySystem.h"
#include "TransformSystem.h"
#include "VulkanRenderer.h"
#include "ReactPhysicsSystem.h"

#include "EntityManager.h"
#include "ComponentManager.h"
#include "SystemManager.h"

#include <chrono>

namespace Puffin
{
	void Engine::MainLoop()
	{
		// Managers
		std::shared_ptr<ECS::EntityManager> EntityManager = std::make_shared<ECS::EntityManager>();
		std::shared_ptr<ECS::ComponentManager> ComponentManager = std::make_shared<ECS::ComponentManager>();
		std::shared_ptr<ECS::SystemManager> SystemManager = std::make_shared<ECS::SystemManager>();
		UI::UIManager UIManager;
		Input::InputManager InputManager;

		SystemManager->componentManager = ComponentManager;

		// Systems
		EntitySystem entitySystem;
		TransformSystem transformSystem;
		std::shared_ptr<Physics::ReactPhysicsSystem> physicsSystem = SystemManager->RegisterSystem<Physics::ReactPhysicsSystem>();
		std::shared_ptr<Rendering::VulkanRenderer> renderSystem = SystemManager->RegisterSystem<Rendering::VulkanRenderer>();

		// Add systems to manager
		

		//AddSystem(&entitySystem);
		//AddSystem(&physicsSystem);
		//AddSystem(&transformSystem);
		//AddSystem(&renderSystem);

		//UIManager.SetEngine(this);

		renderSystem->SetUI(&UIManager);
		renderSystem->SetInputManager(&InputManager);

		transformSystem.SetPhysicsRenderVectors(physicsSystem->GetComponents(), renderSystem->GetComponents());

		for (int i = 0; i < 5; i++)
		{
			uint32_t id = entitySystem.CreateEntity();
			entitySystem.GetEntity(id)->AttachComponent(transformSystem.AddComponent());
			entitySystem.GetEntity(id)->AttachComponent(renderSystem->AddComponent());
		}

		entitySystem.GetEntity(3)->AttachComponent(physicsSystem->AddComponent());
		entitySystem.GetEntity(5)->AttachComponent(physicsSystem->AddComponent());

		running = true;
		playState = PlayState::STOPPED;

		/*for (int i = 0; i < systems.size(); i++)
		{
			systems[i]->Init();
		}*/

		entitySystem.Init();
		transformSystem.Init();
		physicsSystem->Init();
		renderSystem->Init();

		entitySystem.Start();
		transformSystem.Start();
		physicsSystem->Start();
		renderSystem->Start();

		auto lastTime = std::chrono::high_resolution_clock::now(); // Time Count Started
		auto currentTime = std::chrono::high_resolution_clock::now();

		while (running)
		{
			lastTime = currentTime;
			currentTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> duration = currentTime - lastTime;
			float delta_time = duration.count();

			entitySystem.Update(delta_time);
			transformSystem.Update(delta_time);
			physicsSystem->Update(delta_time);
			running = renderSystem->Update(delta_time);

			//Update(delta_time);
		}
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