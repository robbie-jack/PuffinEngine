#pragma once

#ifndef UI_WINDOW_VIEWPORT_H
#define UI_WINDOW_VIEWPORT_H

#include "UIWindow.h"
#include "VKTypes.h"

//#include <vulkan/vulkan.h>

namespace Puffin
{
	namespace UI
	{
		class UIWindowViewport : public UIWindow
		{
		public:

			bool Draw(ImTextureID textureID);

			inline ImVec2 GetViewportSize() { return viewportSize; }

		private:

			ImVec2 viewportSize;
		};
	}
}

#endif // UI_WINDOW_VIEWPORT_H