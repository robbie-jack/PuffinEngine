#pragma once

#ifndef ENGINE_H
#define ENGINE_H

#include <ECS/ECS.h>
#include <SerializeScene.h>
#include <ProjectSettings.h>

#include <GLFW/glfw3.h>

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

		void Play();
		void Restart();
		void Exit();

		inline PlayState GetPlayState() { return playState; };
		inline IO::SceneData& GetScene() { return sceneData; };
		inline ProjectSettings& GetProjectSettings() { return settings; };

	private:

		bool running, restarted;
		PlayState playState;

		ProjectSettings settings;
		IO::SceneData sceneData;

		void DefaultScene(std::shared_ptr<ECS::World> world);
	};

}

#endif // !ENGINE_H