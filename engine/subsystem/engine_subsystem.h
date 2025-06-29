#pragma once

#include "subsystem/subsystem.h"
#include "subsystem/subsystem_reflection.h"

namespace puffin
{
	namespace core
	{
		class EngineSubsystem : public Subsystem
		{
		public:

			EngineSubsystem(std::shared_ptr<Engine> engine);
			~EngineSubsystem() override;

			/*
			 * Update method, called once a frame
			 * If gameplay subsystem only called when game is playing
			 */
			virtual void Update(double deltaTime);

			/*
			 * Whether update method should be called, defaults to false
			 */
			virtual bool ShouldUpdate();

			std::string_view GetName() const override;

		private:



		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<core::EngineSubsystem>()
		{
			return "EngineSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<core::EngineSubsystem>()
		{
			return entt::hs(GetTypeString<core::EngineSubsystem>().data());
		}

		template<>
		inline void RegisterType<core::EngineSubsystem>()
		{
			auto meta = entt::meta<core::EngineSubsystem>()
			.base<core::Subsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}