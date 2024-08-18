#pragma once

#include <memory>

namespace puffin::core
{
	class Engine;

	/* 
	 * Entry Point for Puffin Engine Apps/Games
	 * Prefer implementing functionality in subsystems, but simple app/game logic can be done here instead
	 */
	class Application
	{
	public:
		explicit Application(std::shared_ptr<Engine> engine);
		virtual ~Application();

		/*
		 * Register any application specific subsystems here, gets called before initialization
		 */
		virtual void RegisterSubsystems();

		/*
		 * Application initialization occurs here, called after engine subsystems are initialized
		 */
		virtual void Initialize();

		/*
		 * Application Deinitialization occurs here, called before engine subsystems are deinitialized
		 */
		virtual void Deinitialize();

		/*
		 * Called when gameplay begins
		 */
		virtual void BeginPlay();

		/*
		 * Called when gameplay ends
		 */
		virtual void EndPlay();

		/*
		 * Update method, called once a frame while game is playing
		 */
		virtual void Update(double deltaTime);

		/*
		 * Whether update method should be called, defaults to false
		 */
		virtual bool ShouldUpdate();

		/*
		 * Fixed update method, called once every fixed physics tick while game is playing
		 */
		virtual void FixedUpdate(double fixedTimeStep);

		/*
		 * Whether fixed update method should be called, defaults to false
		 */
		virtual bool ShouldFixedUpdate();

		/*
		 * Update method, called once a frame
		 */
		virtual void EngineUpdate(double deltaTime);

		/*
		 * Whether engine update method should be called, defaults to false
		 */
		virtual bool ShouldEngineUpdate();

	protected:

		std::shared_ptr<Engine> mEngine = nullptr;

	};
}
