#pragma once

#include "subsystem/engine_subsystem.h"
#include "types/size.h"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace window
	{
		class Window;

		class WindowSubsystem : public core::EngineSubsystem
		{
		public:

			explicit WindowSubsystem(const std::shared_ptr<core::Engine>& engine);
			~WindowSubsystem() override;

			void Update(double deltaTime) override;

			std::string_view GetName() const override;

			[[nodiscard]] Window* GetPrimaryWindow() const;

			[[nodiscard]] bool ShouldPrimaryWindowClose() const;
			[[nodiscard]] Size GetPrimaryWindowSize() const;
			[[nodiscard]] uint32_t GetPrimaryWindowWidth() const;
			[[nodiscard]] uint32_t GetPrimaryWindowHeight() const;

			[[nodiscard]] bool GetPrimaryWindowFullscreen() const;
			void SetPrimaryWindowFullscreen(bool fullscreen) const;

			[[nodiscard]] bool GetPrimaryWindowBorderless() const;
			void SetPrimaryWindowBorderless(bool borderless) const;

		protected:

			Window* m_primaryWindow = nullptr;

		private:

			

		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<window::WindowSubsystem>()
		{
			return "WindowSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<window::WindowSubsystem>()
		{
			return entt::hs(GetTypeString<window::WindowSubsystem>().data());
		}

		template<>
		inline void RegisterType<window::WindowSubsystem>()
		{
			auto meta = entt::meta<window::WindowSubsystem>()
				.base<core::EngineSubsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}