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
		ECS::EntityManager EntityManager;
		ECS::ComponentManager ComponentManager;

		UIManager.SetEntityManager(&EntityManager);

		// Systems
		//EntitySystem entitySystem;
		//TransformSystem transformSystem;
		Physics::ReactPhysicsSystem physicsSystem;
		Rendering::VulkanRenderer renderSystem;

		//AddSystem(&entitySystem);
		//AddSystem(&physicsSystem);
		//AddSystem(&transformSystem);
		//AddSystem(&renderSystem);

		//UIManager.SetEngine(this);

		//renderSystem.SetUI(&UIManager);
		//renderSystem.SetInputManager(&InputManager);

		//transformSystem.SetPhysicsRenderVectors(physicsSystem.GetComponents(), renderSystem.GetComponents());

		ComponentManager.RegisterComponent<TransformComponent>();

		for (int i = 0; i < 5; i++)
		{
			ECS::Entity entity = EntityManager.CreateEntity();
			//uint32_t id = entitySystem.CreateEntity();
			//entitySystem.GetEntity(id)->AttachComponent(transformSystem.AddComponent());
			//entitySystem.GetEntity(id)->AttachComponent(renderSystem.AddComponent());
			//transformSystem.AddComponent();
			renderSystem.AddComponent();
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
		renderSystem.Init(&UIManager, &InputManager);

		//entitySystem.Start();
		//transformSystem.Start();
		physicsSystem.Start();
		renderSystem.Start();

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
			running = renderSystem.Update(&UIManager, &InputManager, delta_time);

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