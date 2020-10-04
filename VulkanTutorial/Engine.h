#pragma once

#include "System.h"

#include <vector>

namespace Puffin
{
	enum class PlayState
	{
		STOPPED, // Game is stopped, no physics or game code is begin run
		PLAYING, // Game is playing, all systems being updated
		PAUSED // Game
	};

	class Engine
	{
	public:

		void MainLoop();
		void AddSystem(System* sys);

		void Play();
		void Stop();

		inline PlayState GetPlayState() { return playState; };

	private:

		void Update(float dt);
		void Start();

		std::vector<System*> systems;

		bool running;
		PlayState playState;
	};

}