#pragma once

#ifndef ENGINE_H
#define ENGINE_H

#include <ECS/ECS.h>
#include <SerializeScene.h>
#include <ProjectSettings.h>
#include "Engine/Subsystem.hpp"

#include <glfw/glfw3.h>

#include <vector>
#include <filesystem>
#include <memory>
#include <unordered_map>

namespace fs = std::filesystem;

namespace Puffin
{
	namespace Audio
	{
		class AudioManager;
	}
}

namespace Puffin::Core
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

	class Engine : std::enable_shared_from_this<Engine>
	{
	public:

		Engine() = default;

		void Init();
		void MainLoop();
		bool Update();
		void Destroy();

		void Play();
		void Restart();
		void Exit();

		// Subsystem Methods
		template<typename SubsystemT>
		std::shared_ptr<SubsystemT> RegisterSubsystem()
		{
			const char* typeName = typeid(SubsystemT).name();

			assert(m_subsystemTypes.find(typeName) == m_subsystemTypes.end() && "Registering subsystem more than once");

			// Create subsystem pointer
			std::shared_ptr<SubsystemT> subsystem = std::make_shared<SubsystemT>();
			std::shared_ptr<Subsystem> subsystemBase = std::static_pointer_cast<Subsystem>(subsystem);
			subsystemBase->SetEngine(shared_from_this());

			// Cast subsystem to Subsystem parent and add to subsystems map
			m_subsystems.insert({ typeName, subsystemBase });

			return subsystem;
		}

		template<typename SubsystemT>
		std::shared_ptr<SubsystemT> GetSubsystem()
		{
			const char* typeName = typeid(SubsystemT).name();

			assert(m_subsystemTypes.find(typeName) != m_subsystemTypes.end() && "Subsystem used before registering.");

			return std::static_pointer_cast<SubsystemT>(m_subsystems[typeName]);
		}

		PlayState GetPlayState() const { return playState; }
		inline std::shared_ptr<IO::SceneData> GetScene() { return m_sceneData; }

		inline IO::ProjectSettings& GetProjectSettings() { return settings; }

	private:

		GLFWwindow* m_window = nullptr;
		GLFWmonitor* m_monitor = nullptr;

		// Subsystems
		std::shared_ptr<Audio::AudioManager> m_audioManager = nullptr;

		bool running = true;
		PlayState playState = PlayState::STOPPED;

		IO::ProjectFile projectFile;

		IO::ProjectSettings settings;

		std::shared_ptr<IO::SceneData> m_sceneData = nullptr;

		// Subsystem Members
		std::unordered_map<const char*, std::shared_ptr<Core::Subsystem>> m_subsystems;

		void AddDefaultAssets();

		void DefaultScene(std::shared_ptr<ECS::World> world);
		void PhysicsScene(std::shared_ptr<ECS::World> world);

	};
}

#endif // !ENGINE_H