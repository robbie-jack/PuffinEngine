#include "Engine.h"

#include <chrono>

namespace Puffin
{
	void Engine::MainLoop()
	{
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
		if (playState == PlayState::STOPPED)
		{
			for (int i = 0; i < systems.size(); i++)
			{
				systems[i]->Start();
			}

			playState = PlayState::PLAYING;
		}
	}

	void Engine::Stop()
	{
		if (playState == PlayState::PLAYING || playState == PlayState::PAUSED)
		{
			for (int i = 0; i < systems.size(); i++)
			{
				systems[i]->Stop();
			}

			playState = PlayState::STOPPED;
		}
	}

	void Engine::PlayPause()
	{
		if (playState == PlayState::PLAYING)
		{
			playState = PlayState::PAUSED;
		}
		else if (playState == PlayState::PAUSED)
		{
			playState = PlayState::PLAYING;
		}
	}
}