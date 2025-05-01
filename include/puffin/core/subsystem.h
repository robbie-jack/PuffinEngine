#pragma once

#include <memory>
#include <string>

namespace puffin::core
{
	class Engine;
	class SubsystemManager;

	enum class SubsystemType
	{
		Engine = 0, // Subsystem shares lifetime of engine
		Editor, // Subsystem shares lifetime of engine, is only initialized when editor is active
		Gameplay, // Subsystem shares lifetime of gameplay
		Window, // Unique subsystem type, manages window creation and deletion, same lifetime as engine subsystem
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
		 * Called when a subsystems is registered to register any additional types (subsystems, components, nodes, etc...)
		 */
		virtual void RegisterTypes();

		/*
		 * All subsystem initialization occurs here
		 * subsystem initialization is called depending on what type of subsystem this is
		 */
		virtual void Initialize(SubsystemManager* subsystemManager);

		/*
		 * All subsystem deinitialization occurs here, same rules apply as initialization method for method execution
		 */
		virtual void Deinitialize();

		[[nodiscard]] virtual SubsystemType GetType() const;

		/*
		 * Called when gameplay begins
		 */
		virtual void BeginPlay();

		/*
		 * Called when gameplay ends
		 */
		virtual void EndPlay();

		/*
		 * Update method, called once a frame
		 * If gameplay subsystem only called when game is playing
		 */
		virtual void Update(double deltaTime);

		/*
		 * Whether update method should be called, defaults to false
		 */
		virtual bool ShouldUpdate();

		/*
		 * Fixed update method, called once every fixed physics tick, and only on gameplay subsystems
		 */
		virtual void FixedUpdate(double fixedTimeStep);

		/*
		 * Whether fixed update method should be called, defaults to false
		 */
		virtual bool ShouldFixedUpdate();

		/*
		 * Called once a frame on input subsystem
		 */
		virtual void ProcessInput();

		const std::string& GetName();

	protected:

		std::shared_ptr<Engine> mEngine = nullptr;
		std::string mName = "Subsystem";

	};
}
