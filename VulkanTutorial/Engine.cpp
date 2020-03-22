#include "Engine.h"

#include <chrono>

void Engine::MainLoop()
{
	running = true;

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

		running = Update(delta_time);
	}
}

bool Engine::Update(float dt)
{
	for (int i = 0; i < systems.size(); i++)
	{
		systems[i]->Update(dt);
	}

	return true;
}

void Engine::AddSystem(System* sys)
{
	systems.push_back(sys);
}