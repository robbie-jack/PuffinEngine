#pragma once

#include "puffin/core/application.h"
#include "puffin/project_settings.h"
#include "puffin/core/system.h"
#include "argparse/argparse.hpp"

#include <GLFW/glfw3.h>

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace puffin
{
    void add_default_engine_arguments(argparse::ArgumentParser &parser);

	/*namespace audio
	{
		class AudioSubsystem;
	}*/

	namespace ui
	{
		class UISubsystem;
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
		Started,		// Game has just started, gameplay systems need to be initialized
		Playing,		// Game is playing, all systems being updated
		JustStopped,	// Game has just been stopped, perform all system stop functions
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

		Engine() = default;
		~Engine() = default;

		void setup(const argparse::ArgumentParser &parser);

		void startup();
		bool update();
		void shutdown();

		void play();
		void restart();
		void exit();

		template<typename AppT>
		void register_app()
		{
			assert(m_application == nullptr && "Registering multiple applications");

			m_application = std::static_pointer_cast<Application>(std::make_shared<AppT>(shared_from_this()));
		}

		// System/Subsystem Methods
		template<typename SubsystemT>
		std::shared_ptr<SubsystemT> register_system()
		{
			const char* typeName = typeid(SubsystemT).name();

			assert(m_systems.find(typeName) == m_systems.end() && "Engine::registerSystem() - Registering system more than once");

			// Create subsystem pointer
			auto subsystem = std::make_shared<SubsystemT>(shared_from_this());
			auto subsystemBase = std::static_pointer_cast<System>(subsystem);

			// Cast subsystem to Subsystem parent and add to subsystems map
			m_systems.insert({ typeName, subsystemBase });

			return subsystem;
		}

		template<typename SubsystemT>
		std::shared_ptr<SubsystemT> get_system()
		{
			const char* typeName = typeid(SubsystemT).name();

			assert(m_systems.find(typeName) != m_systems.end() && "Engine::getSystem() - System used before registering.");

			return std::static_pointer_cast<SubsystemT>(m_systems[typeName]);
		}

		// Register a callback function to be executed during engine runtime
		void register_callback(ExecutionStage executionStage, const std::function<void()>& callback, const std::string& name = "", const int& priority = 100)
		{
			// Add function handler to vector
			m_registered_callbacks[executionStage].emplace_back(callback, name, priority);

			// Sort vector by priority
			std::sort(m_registered_callbacks[executionStage].begin(), m_registered_callbacks[executionStage].end());
		}

		PlayState play_state() const { return m_play_state; }

		bool should_render_editor_ui() const { return m_should_render_editor_ui; }

		const double& time_step_fixed() const { return m_time_step_fixed; }

		void set_time_step_fixed(const double timeStepFixed) { m_time_step_fixed = timeStepFixed; }

		const double& delta_time() const { return m_delta_time; }

		const double& accumulated_time() const { return m_accumulated_time; }

		void update_delta_time(double sampled_time);

		void set_load_scene_on_launch(const bool load_scene_on_launch) { m_load_scene_on_launch = load_scene_on_launch; }
		void set_load_engine_default_scene(const bool load_engine_default_scene) { m_setup_engine_default_scene = load_engine_default_scene; }

		double get_stage_execution_time_last_frame(const core::ExecutionStage& updateOrder)
		{
			return m_stage_execution_time_last_frame[updateOrder];
		}

		const std::unordered_map<std::string, double>& getCallbackExecutionTimeForUpdateStageLastFrame(const core::ExecutionStage& updateOrder)
		{
			return m_callback_execution_time_last_frame[updateOrder];
		}

	private:

		bool m_running = true;
		bool m_load_scene_on_launch = false;
		bool m_setup_engine_default_scene = false;
		bool m_setup_engine_default_settings = false;
		bool m_should_limit_framerate = true; // Whether framerate should be capped at m_frameRateMax
		bool m_should_track_execution_time = true; // Should track time to execute callback/stages
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

		// System/Subsystem Members
		std::unordered_map<const char*, std::shared_ptr<core::System>> m_systems;
		std::unordered_map<core::ExecutionStage, std::vector<EngineCallbackHandler>> m_registered_callbacks; // Map of callback functions registered for execution

		std::unordered_map<core::ExecutionStage, double> m_stage_execution_time; // Map of time it takes each stage of engine to execute (Physics, Rendering, Gameplay, etc...)
		std::unordered_map<core::ExecutionStage, std::unordered_map<std::string, double>> m_callback_execution_time; // Map of time it takes for each system to execute

		std::unordered_map<core::ExecutionStage, double> m_stage_execution_time_last_frame;
		std::unordered_map<core::ExecutionStage, std::unordered_map<std::string, double>> m_callback_execution_time_last_frame;

		io::ProjectFile m_project_file;

		// Execute callbacks for this execution stage
		void execute_callbacks(const core::ExecutionStage& execution_stage, const bool should_track_execution_time = false)
		{
			double startTime = 0.0, endTime = 0.0;
			double stageStartTime = 0.0;

			if (m_should_track_execution_time && should_track_execution_time)
			{
				stageStartTime = glfwGetTime();
			}

			for (const auto& callback : m_registered_callbacks[execution_stage])
			{
				if (m_should_track_execution_time && should_track_execution_time)
				{
					startTime = glfwGetTime();
				}

				callback.execute();

				if (m_should_track_execution_time && should_track_execution_time)
				{
					endTime = glfwGetTime();

					if (m_callback_execution_time[execution_stage].count(callback.name()) == 0)
					{
						m_callback_execution_time[execution_stage][callback.name()] = 0.0;
					}

					m_callback_execution_time[execution_stage][callback.name()] += endTime - startTime;
				}
			}

			if (m_should_track_execution_time && should_track_execution_time)
			{
				const double stageEndTime = glfwGetTime();

				if (m_stage_execution_time.count(execution_stage) == 0)
				{
					m_stage_execution_time[execution_stage] = 0.0;
				}

				m_stage_execution_time[execution_stage] += (stageEndTime - stageStartTime);
			}
		}

		void update_execution_time()
		{
			if (m_should_track_execution_time)
			{
				m_stage_execution_time_last_frame.clear();
				m_callback_execution_time_last_frame.clear();

				m_stage_execution_time_last_frame = m_stage_execution_time;
				m_callback_execution_time_last_frame = m_callback_execution_time;

				m_stage_execution_time.clear();
				m_callback_execution_time.clear();
			}
		}

		void add_default_assets();
		void reimport_default_assets();
		void load_and_resave_assets();
        void default_settings();

		void default_scene();
		void physics_scene_3d();
		void procedural_scene();

	};
}
