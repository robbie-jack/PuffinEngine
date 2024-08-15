#pragma once

#include "puffin/core/application.h"
#include "puffin/projectsettings.h"
#include "puffin/core/subsystem.h"
#include "argparse/argparse.hpp"

#include <GLFW/glfw3.h>

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

#include "subsystemmanager.h"

namespace fs = std::filesystem;

namespace puffin
{
    void add_default_engine_arguments(argparse::ArgumentParser &parser);
}

namespace puffin::core
{
	enum class PlayState
	{
		BeginPlay,		// Game has just started, gameplay systems need to be initialized
		Playing,		// Game is playing, all systems being updated
		EndPlay,	// Game has just been stopped, perform all system stop functions
		Stopped,		// Game is stopped, no physics or game code is begin run, all data is in default state
		JustPaused,		// Game has just been paused
		Paused,			// Game is paused, systems not being updated,
		JustUnpaused	// Game has just been unpaused
	};

	class Engine : public std::enable_shared_from_this<Engine>
	{
	public:

		Engine();
		~Engine();

		void setup();
		void initialize(const argparse::ArgumentParser& parser);
		bool update();
		void deinitialize();

		void play();
		void restart();
		void exit();

		template<typename AppT>
		void register_app()
		{
			assert(m_application == nullptr && "Registering multiple applications");

			m_application = std::static_pointer_cast<Application>(std::make_shared<AppT>(shared_from_this()));
		}

		PlayState play_state() const { return m_play_state; }

		bool setup_engine_default_settings() const { return m_setup_engine_default_settings; }
		bool should_render_editor_ui() const { return m_should_render_editor_ui; }

		const double& time_step_fixed() const { return m_time_step_fixed; }

		void set_time_step_fixed(const double timeStepFixed) { m_time_step_fixed = timeStepFixed; }

		const double& delta_time() const { return m_delta_time; }

		const double& accumulated_time() const { return m_accumulated_time; }

		template<typename T>
		void register_subsystem() const
		{
			m_subsystem_manager->register_subsystem<T>();
		}

		template<typename T>
		T* get_subsystem() const
		{
			return m_subsystem_manager->get_subsystem<T>();
		}

	private:

		bool m_running = true;
		bool m_load_scene_on_launch = false;
		bool m_setup_engine_default_scene = false;
		bool m_setup_engine_default_settings = false;
		bool m_should_limit_framerate = true; // Whether framerate should be capped at m_frameRateMax
		bool m_should_render_editor_ui = true; // Whether editor UI should be rendered

		PlayState m_play_state = PlayState::Stopped;

		// Framerate Members
		uint16_t m_frame_rate_max = 0; // Limit on how fast game runs
		uint16_t m_physics_ticks_per_frame = 60; // How many times physics code should run per frame

		// Time Members
		double m_last_time = 0.0;
		double m_current_time = 0.0;
		double m_delta_time = 0.0; // How long it took last frame to complete
		double m_accumulated_time = 0.0; // Time passed since last physics tick
		double m_time_step_fixed = 1.0 / m_physics_ticks_per_frame; // How often deterministic code like physics should occur (defaults to 60 times a second)
		double m_time_step_limit = 1 / 25.0; // Maximum amount of time each frame should take to complete

		std::shared_ptr<Application> m_application = nullptr;
		std::unique_ptr<SubsystemManager> m_subsystem_manager = nullptr;

		io::ProjectFile m_project_file;

		void register_required_subsystems() const;

		void add_default_assets();
		void reimport_default_assets();
		void load_and_resave_assets();

		void default_scene();
		void physics_scene_3d();
		void procedural_scene();

		void update_delta_time(double sampled_time);

	};
}
