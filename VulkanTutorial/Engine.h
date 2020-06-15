#pragma once

#include "System.h"

#include <vector>

class Engine
{
public:

	void MainLoop();
	void AddSystem(System *sys);

private:

	void Update(float dt);

	std::vector<System*> systems;

	bool running;
};