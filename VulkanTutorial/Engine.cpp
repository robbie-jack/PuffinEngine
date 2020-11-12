#include "Engine.h"

#include "EntitySystem.h"
#include "TransformSystem.h"
#include "VulkanRenderer.h"
#include "ReactPhysicsSystem.h"

#include <chrono>

namespace Puffin
{
	void Engine::MainLoop()
	{
		EntitySystem entitySystem;
		TransformSystem transformSystem;
		Physics::ReactPhysicsSystem physicsSystem;
		Rendering::VulkanRenderer renderSystem;

		UI::UIManager UIManager;
		Input::InputManager InputManager;

		AddSystem(&entitySystem);
		AddSystem(&physicsSystem);
		AddSystem(&transformSystem);
		AddSystem(&renderSystem);

		//UIManager.SetEngine(this);

		renderSystem.SetUI(&UIManager);
		renderSystem.SetInputManager(&InputManager);

		transformSystem.SetPhysicsRenderVectors(physicsSystem.GetComponents(), renderSystem.GetComponents());

		for (int i = 0; i < 5; i++)
		{
			uint32_t id = entitySystem.CreateEntity();
			entitySystem.GetEntity(id)->AttachComponent(transformSystem.AddComponent());
			entitySystem.GetEntity(id)->AttachComponent(renderSystem.AddComponent());
		}

		entitySystem.GetEntity(3)->AttachComponent(physicsSystem.AddComponent());
		entitySystem.GetEntity(5)->AttachComponent(physicsSystem.AddComponent());

		running = true;
		playState = PlayState::STOPPED;

		for (int i = 0; i < systems.size(); i++)
		{
			systems[i]->Init();
		}

		auto lastTime = std::chrono::high_resolution_clock::now(); // Time Count Started
		auto currentTime = std::chrono::high_resolution_clock::now();

		while (running)
		{
			lastTime = currentTime;
			currentTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> duration = currentTime - lastTime;
			float delta_time = duration.count();

			Update(delta_time);
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
		for (int i = 0; i < systems.size(); i++)
		{
			systems[i]->Start();
		}

		playState = PlayState::PLAYING;
	}

	void Engine::Stop()
	{
		if (playState == PlayState::PLAYING | playState == PlayState::PAUSED)
		{
			for (int i = 0; i < systems.size(); i++)
			{
				systems[i]->Stop();
			}

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