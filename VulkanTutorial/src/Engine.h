#pragma once

#ifndef ENGINE_H
#define ENGINE_H

#include <ECS/ECS.h>
#include <SerializeScene.h>
#include <ProjectSettings.h>

#include <GLFW/glfw3.h>

#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace Puffin
{
	enum class PlayState
	{
		STARTED, // Game has just started, gameplay systems need to be initialized
		PLAYING, // Game is playing, all systems being updated
		STOPPED, // Game is stopped, no physics or game code is begin run, all data is in default state
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

		inline IO::ProjectSettings& GetProjectSettings() { return settings; };

	private:

		bool running, restarted;
		PlayState playState;

		IO::ProjectFile projectFile;

		IO::ProjectSettings settings;

		IO::SceneData sceneData;

		void DefaultScene(std::shared_ptr<ECS::World> world);
		void PhysicsScene(std::shared_ptr<ECS::World> world);

	};

}

#endif // !ENGINE_H