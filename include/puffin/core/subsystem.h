#pragma once

#include <memory>

namespace puffin::core
{
	class Engine;
	class SubsystemManager;

	enum class SubsystemType
	{
		Engine = 0, // Subsystem shares lifetime of engine
		Editor, // Subsystem shares lifetime of engine, is only initialized when editor is active
		Gameplay, // Subsystem shares lifetime of gameplay
		Input, // Unique subsystem type, which processes system input, same lifetime as engine subsystem
		Render // Unique subsystem type, which handles scene & editor rendering, same lifetime as engine subsystem
	};

	/*
	 * Subsystem is a class that is responsible for some defined functionality within puffin
	 * i.e rendering, physics, window management, input, etc..
	 */
	class Subsystem
	{
	public:

		explicit Subsystem(std::shared_ptr<Engine> engine);
		virtual ~Subsystem();

		/*
		 * All subsystem initialization occurs here
		 * subsystem initialization is called depending on what type of subsystem this is
		 */
		virtual void initialize(SubsystemManager* subsystem_manager);

		/*
		 * All subsystem deinitialization occurs here, same rules apply as initialization method for method execution
		 */
		virtual void deinitialize();

		[[nodiscard]] virtual SubsystemType type() const;

		/*
		 * Called when gameplay begins
		 */
		virtual void begin_play();

		/*
		 * Called when gameplay ends
		 */
		virtual void end_play();

		/*
		 * Update method, called once a frame
		 * If gameplay subsystem only called when game is playing
		 */
		virtual void update(double delta_time);

		/*
		 * Whether update method should be called, defaults to false
		 */
		virtual bool should_update();

		/*
		 * Fixed update method, called once every fixed physics tick, and only on gameplay subsystems
		 */
		virtual void fixed_update(double fixed_time);

		/*
		 * Whether fixed update method should be called, defaults to false
		 */
		virtual bool should_fixed_update();

		/*
		 * Called once a frame on input subsystem
		 */
		virtual void process_input();

		/*
		 * Called each frame to wait for last presentation to complete and sample frame time
		 */
		virtual double wait_for_last_presentation_and_sample_time();

		/*
		 * Called each frame to render 2d/3d scene to display
		 */
		virtual void render(double delta_time);

	protected:

		std::shared_ptr<Engine> m_engine = nullptr;

	};
}
