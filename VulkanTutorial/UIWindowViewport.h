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

			inline void SetTextureSampler(VkSampler sampler) { textureSampler = sampler; };
			inline void SetSceneTexture(Puffin::Rendering::Texture texture) { sceneTexture = texture; };

		private:

			// Vulkan Texture Sampler for Rendering Textures
			VkSampler textureSampler;

			// Texture Containing Rendered Scene
			Puffin::Rendering::Texture sceneTexture;
		};
	}
}