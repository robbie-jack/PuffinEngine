#pragma once

#include "System.h"

#include <vector>

namespace Puffin
{
	enum class PlayState
	{
		STOPPED, // Game is stopped, no physics or game code is begin run
		STARTING, // Game is is being started, relevant systems are being initialized
		PLAYING, // Game is playing, all systems being updated
		PAUSED // Game
	};

	class Engine
	{
	public:

		void MainLoop();
		void AddSystem(System* sys);
		void Start();
		void Stop();
		void PlayPause();

	private:

		void Update(float dt);

		std::vector<System*> systems;

		bool running;
		PlayState playState;
	};

}