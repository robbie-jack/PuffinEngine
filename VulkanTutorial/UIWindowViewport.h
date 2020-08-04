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
		private:
			VkSampler textureSampler;
			Puffin::Rendering::Texture sceneTexture;
		};
	}
}