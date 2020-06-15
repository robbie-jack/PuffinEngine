#include "Engine.h"

#include <chrono>

void Engine::MainLoop()
{
	running = true;

	for (int i = 0; i < systems.size(); i++)
	{
		systems[i]->Init();
	}

	//inputManager.SetupInput();

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
	for (int i = 0; i < systems.size(); i++)
	{
		if (systems[i]->Update(dt) == false)
		{
			running = false;
		}
	}
}

void Engine::AddSystem(System* sys)
{
	systems.push_back(sys);
}