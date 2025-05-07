#pragma once

#include <cstdint>
#include <string>

namespace puffin
{
	struct Size;

	namespace core
	{
		class Engine;
	}

	namespace window
	{
		class Window
		{
		public:

			Window(int32_t width, int32_t height, const std::string& name = "Puffin Engine", uint32_t flags = 0);
			virtual ~Window() = default;

			[[nodiscard]] virtual bool ShouldClose() const = 0;

			[[nodiscard]] virtual Size GetSize() const = 0;
			[[nodiscard]] virtual uint32_t GetWidth() const = 0;
			[[nodiscard]] virtual uint32_t GetHeight() const = 0;

			[[nodiscard]] virtual bool GetIsResized() const = 0;

			[[nodiscard]] virtual bool GetFullscreen() const = 0;
			virtual void SetFullscreen(bool fullscreen) = 0;

			[[nodiscard]] virtual bool GetBorderless() const = 0;
			virtual void SetBorderless(bool borderless) = 0;

		};
	}
}
