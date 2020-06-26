#pragma once

#include "System.h"

#include <vector>

namespace Puffin
{
	class Engine
	{
	public:

		void MainLoop();
		void AddSystem(System* sys);

	private:

		void Update(float dt);

		std::vector<System*> systems;

		bool running;
	};

}