#pragma once

#include <memory>
#include <string>

#include "subsystem/subsystem_reflection.h"

namespace puffin
{
	namespace core
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
			 * Called immediately prior to engine, app and subsystem initialization. Type registration (Subsystems, nodes, components, etc...) should occur here
			 */
			virtual void PreInitialize();

			/*
			 * All subsystem initialization occurs here
			 * subsystem initialization is called depending on what type of subsystem this is
			 */
			virtual void Initialize(SubsystemManager* subsystemManager);

			/*
			 * Called immediately after engine, app and subsystem initialization, prior to scene loading.
			 */
			virtual void PostInitialize();

			/*
			 * All subsystem deinitialization occurs here, same rules apply as initialization method for method execution
			 */
			virtual void Deinitialize();

			/*
			 *	Called at end of engine initialization, use for any late initialization logic
			 */
			virtual void PostSceneLoad();

			/*
			 * Called when gameplay begins
			 */
			virtual void BeginPlay();

			/*
			 * Called when gameplay ends
			 */
			virtual void EndPlay();

			virtual std::string_view GetName() const;

		protected:

			std::shared_ptr<Engine> mEngine = nullptr;
			bool mInitialized = false;

		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<core::Subsystem>()
		{
			return "Subsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<core::Subsystem>()
		{
			return entt::hs(GetTypeString<core::Subsystem>().data());
		}

		template<>
		inline void RegisterType<core::Subsystem>()
		{
			auto meta = entt::meta<core::Subsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}