#pragma once

#include <memory>
#include <string>

namespace puffin::core
{
	class Engine;

	/*
	 * Return time since engine launch in seconds
	 */
	double GetTime();

	class Platform
	{
	public:

		Platform(std::shared_ptr<Engine> engine);
		virtual ~Platform() = default;

		/*
		 * Called immediately prior to engine, app and subsystem initialization. Type registration (Subsystems, nodes, components, etc...) should occur here
		 */
		virtual void PreInitialize() = 0;

		/*
		 * Application initialization occurs here, called after engine subsystems are initialized
		 */
		virtual void Initialize() = 0;

		/*
		 * Called immediately after engine, app and subsystem initialization, prior to scene loading.
		 */
		virtual void PostInitialize() = 0;

		/*
		 * Application Deinitialization occurs here, called before engine subsystems are deinitialized
		 */
		virtual void Deinitialize() = 0;

	protected:

		std::string mName;
		std::shared_ptr<Engine> mEngine = nullptr;

	private:



	};
}
