#pragma once

#include "System.h"

#include <vector>

class Engine
{
public:

	void Update(float dt);
	void MainLoop();
	void AddSystem(System *sys);

private:
	std::vector<System*> systems;
	bool running;
};