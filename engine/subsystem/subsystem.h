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
			virtual void Initialize(core::SubsystemManager* subsystemManager);

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

			/*
			 *	Return subsystem type name
			 */
			virtual std::string_view GetName() const;

		protected:

			std::shared_ptr<Engine> m_engine = nullptr;

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