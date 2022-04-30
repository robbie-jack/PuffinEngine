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
		STARTED,		// Game has just started, gameplay systems need to be initialized
		PLAYING,		// Game is playing, all systems being updated
		JUST_STOPPED,	// Game has just been stopped, perform all system stop functions
		STOPPED,		// Game is stopped, no physics or game code is begin run, all data is in default state
		JUST_PAUSED,	// Game has just been paused
		PAUSED,			// Game is paused, systems not being updated,
		JUST_UNPAUSED	// Game has just been unpaused
	};

	class Engine
	{
	public:

		void MainLoop();

		void Play();
		void Restart();
		void Exit();

		inline PlayState GetPlayState() const { return playState; }
		inline std::shared_ptr<IO::SceneData> GetScene() { return m_sceneData; }

		inline IO::ProjectSettings& GetProjectSettings() { return settings; }

	private:

		bool running;
		PlayState playState;

		IO::ProjectFile projectFile;

		IO::ProjectSettings settings;

		std::shared_ptr<IO::SceneData> m_sceneData = nullptr;

		void AddDefaultAssets();

		void DefaultScene(std::shared_ptr<ECS::World> world);
		void PhysicsScene(std::shared_ptr<ECS::World> world);

	};

}

#endif // !ENGINE_H