#pragma once

#include "UIWindow.h"
#include "Texture.h"

//#include <vulkan/vulkan.h>

namespace Puffin
{
	namespace UI
	{
		class UIWindowViewport : public UIWindow
		{
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

			inline void SetSceneTexture(Puffin::Rendering::Texture texture) { sceneTexture = texture; };
			inline ImVec2 GetViewportSize() { return viewportSize; }

		private:

			// Texture Containing Rendered Scene
			Puffin::Rendering::Texture sceneTexture;
			ImVec2 viewportSize;
		};
	}
}