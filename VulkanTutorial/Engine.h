#pragma once

#include "ECS.h"

#include <vector>

namespace Puffin
{
	enum class PlayState
	{
		STOPPED, // Game is stopped, no physics or game code is begin run, all data is in default state
		PLAYING, // Game is playing, all systems being updated
		PAUSED // Game is paused, systems not being updated
	};

	class Engine
	{
	public:

		void MainLoop();

		void DefaultScene(ECS::World* world);

		void Play();
		void Stop();

		inline PlayState GetPlayState() { return playState; };

	private:

		bool running;
		PlayState playState;
	};

}