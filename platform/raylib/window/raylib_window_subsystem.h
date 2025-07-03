#pragma once

#include "window/window_subsystem.h"
#include "Window.hpp"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace window
	{
		class RaylibWindowSubsystem : public WindowSubsystem
		{
		public:

			explicit RaylibWindowSubsystem(const std::shared_ptr<core::Engine>& engine);
			~RaylibWindowSubsystem() override;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

			std::string_view GetName() const override;

		private:

			

		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<window::RaylibWindowSubsystem>()
		{
			return "RaylibWindowSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<window::RaylibWindowSubsystem>()
		{
			return entt::hs(GetTypeString<window::RaylibWindowSubsystem>().data());
		}

		template<>
		inline void RegisterType<window::RaylibWindowSubsystem>()
		{
			auto meta = entt::meta<window::RaylibWindowSubsystem>()
				.base<window::WindowSubsystem>()
				.base<core::EngineSubsystem>()
				.base<core::Subsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}