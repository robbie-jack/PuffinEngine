#pragma once
#include "subsystem.h"

namespace puffin
{
	namespace core
	{
		class GameplaySubsystem : public Subsystem
		{
		public:

			GameplaySubsystem(std::shared_ptr<Engine> engine);
			~GameplaySubsystem() override;

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

		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<core::GameplaySubsystem>()
		{
			return "GameplaySubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<core::GameplaySubsystem>()
		{
			return entt::hs(GetTypeString<core::GameplaySubsystem>().data());
		}

		template<>
		inline void RegisterType<core::GameplaySubsystem>()
		{
			auto meta = entt::meta<core::GameplaySubsystem>()
				.base<core::Subsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}
