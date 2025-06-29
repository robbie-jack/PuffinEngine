#pragma once

#include <memory>
#include <string>

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
		explicit Application(std::string name, std::shared_ptr<Engine> engine);
		virtual ~Application();

		/*
		 * Called immediately prior to engine, app and subsystem initialization. Type registration (Subsystems, nodes, components, etc...) should occur here
		 */
		virtual void PreInitialize();

		/*
		 * Application initialization occurs here, called after engine subsystems are initialized
		 */
		virtual void Initialize();

		/*
		 * Called immediately after engine, app and subsystem initialization, prior to scene loading.
		 */
		virtual void PostInitialize();

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

		[[nodiscard]] const std::string& GetName() const;

	protected:

		std::string mName;
		std::shared_ptr<Engine> mEngine = nullptr;

	};
}
