#pragma once

#include "puffin/core/application.h"
#include "puffin/project_settings.h"
#include "puffin/core/subsystem.h"
#include "argparse/argparse.hpp"

#include <GLFW/glfw3.h>

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

#include "subsystem_manager.h"

namespace fs = std::filesystem;

namespace puffin
{
    void add_default_engine_arguments(argparse::ArgumentParser &parser);

	/*namespace audio
	{
		class AudioSubsystem;
	}*/

	namespace core
	{
		template<typename T>
		class SubsystemManager;

		class EngineSubsystem;
		using EngineSubsystemManager = SubsystemManager<EngineSubsystem>;
	}

	namespace editor
	{
		class EditorSubsystem;
		using EditorSubsystemManager = core::SubsystemManager<EditorSubsystem>;
	}

	namespace gameplay
	{
		class GameplaySubsystem;
		using GameplaySubsystemManager = core::SubsystemManager<GameplaySubsystem>;
	}

	namespace physics
	{
		class PhysicsSubsystem;
		using PhysicsSubsystemManager = core::SubsystemManager<PhysicsSubsystem>;
	}

	namespace rendering
	{
		class RenderSubsystem;
		using RenderSubsystemManager = core::SubsystemManager<RenderSubsystem>;
	}

	namespace ui
	{
		class EditorUISubsystem;
	}
}

namespace puffin::core
{
	// Various stages when methods can be executed during engine runtime
	enum class ExecutionStage
	{
		Idle,								// Only used for calculating idle time when frame rate is limited, do not use with callback

		StartupSubsystem,					// Occurs once on engine launch, use for one off subsystem initialization outside of constructor
		Startup,							// Occurs once on engine launch, use for one off system initialization outside of constructor

		BeginPlay,							// Occurs whenever gameplay is started

		UpdateInput,						// Occurs every frame, used for sampling input
		WaitForLastPresentationAndSample,	// Occurs every frame, should only be registered by the active render system to get presentation time
		UpdateSubsystem,					// Occurs every frame, regardless if game is currently playing/paused
		UpdateFixed,						// Updates happen at a fixed rate, and can occur multiple times in a single frame - Useful for physics or code which should be deterministic
		Update,								// Update once a frame - Useful for non-determinstic gameplay code

		Render,								// Update once a frame - Useful for code which relates to the rendering pipeline

		EndPlay,							// Occurs when game play is stopped, use for resetting any gameplay data

		Shutdown,							// Occurs when engine exits, use for cleaning up all system data outside of destructor
		ShutdownSubsystem					// Occurs when engine exits, use for cleaning up all subsystem data outside of destructor
	};

	const std::vector<std::pair<ExecutionStage, const std::string>> gExecutionStageOrder =
	{
		{ ExecutionStage::Idle, "Idle" },
		{ ExecutionStage::UpdateFixed, "UpdateFixed" },
		{ ExecutionStage::Update, "Update" },
		{ ExecutionStage::Render, "Render" },
	};

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

	// Handler class for executing functions in engine
	class EngineCallbackHandler
	{
	public:

		EngineCallbackHandler(const std::function<void()>& callback, const std::string& name, const uint8_t& priority) :
			mCallback(callback), mName(name), mPriority(priority) {}

		void execute() const
		{
			mCallback();
		}

		const std::string& name() const
		{
			return mName;
		}

		bool operator<(const EngineCallbackHandler& other) const
		{
			return mPriority < other.mPriority;
		}

	private:

		std::function<void()> mCallback;
		std::string mName;
		uint8_t mPriority;

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
		void register_engine_subsystem() const
		{
			m_engine_subsystem_manager->register_subsystem<T>();
		}

		template<typename T>
		T* get_engine_subsystem() const
		{
			return m_engine_subsystem_manager->get_subsystem<T>();
		}

		template<typename T>
		void register_editor_subsystem() const
		{
			m_editor_subsystem_manager->register_subsystem<T>();
		}

		template<typename T>
		T* get_editor_subsystem() const
		{
			return m_editor_subsystem_manager->get_subsystem<T>();
		}

		template<typename T>
		void register_render_subsystem() const
		{
			m_render_subsystem_manager->register_subsystem<T>();
		}

		template<typename T>
		T* get_render_subsystem() const
		{
			return m_render_subsystem_manager->get_subsystem<T>();
		}

		template<typename T>
		void register_gameplay_subsystem() const
		{
			m_gameplay_subsystem_manager->register_subsystem<T>();
		}

		template<typename T>
		T* get_gameplay_subsystem() const
		{
			return m_gameplay_subsystem_manager->get_subsystem<T>();
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
		std::shared_ptr<EngineSubsystemManager> m_engine_subsystem_manager = nullptr;
		std::shared_ptr<editor::EditorSubsystemManager> m_editor_subsystem_manager = nullptr;
		std::shared_ptr<rendering::RenderSubsystemManager> m_render_subsystem_manager = nullptr;
		std::shared_ptr<gameplay::GameplaySubsystemManager> m_gameplay_subsystem_manager = nullptr;

		io::ProjectFile m_project_file;

		void register_required_subsystems() const;

		void add_default_assets();
		void reimport_default_assets();
		void load_and_resave_assets();
        void default_settings();

		void default_scene();
		void physics_scene_3d();
		void procedural_scene();

		void update_delta_time(double sampled_time);

	};
}
