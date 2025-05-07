#pragma once

#include "puffin/window/window.h"

namespace raylib
{
	class Window;
}

namespace puffin
{
	namespace window
	{
		class WindowRL : public Window
		{
		public:

			WindowRL(int32_t width, int32_t height, const std::string& name = "Puffin Engine", uint32_t flags = 0);
			~WindowRL() override;

			[[nodiscard]] bool ShouldClose() const override;

			[[nodiscard]] Size GetSize() const override;
			[[nodiscard]] uint32_t GetWidth() const override;
			[[nodiscard]] uint32_t GetHeight() const override;

			[[nodiscard]] bool GetIsResized() const override;

			[[nodiscard]] bool GetFullscreen() const override;
			void SetFullscreen(bool fullscreen) override;

			[[nodiscard]] bool GetBorderless() const override;
			void SetBorderless(bool borderless) override;

		private:

			raylib::Window* mWindow = nullptr;

		};
	}
}