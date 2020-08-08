#pragma once

#include <string>
#include <vulkan/vulkan.h>

#include "imgui/imgui.h"
#include "InputManager.h"

namespace Puffin
{
	namespace UI
	{
		class UIWindow
		{
		public:

			UIWindow();
			~UIWindow();

			virtual bool Draw(float dt, Puffin::Input::InputManager* InputManager);

			void Show();

			inline bool* GetShow() { return &show; };
			inline std::string GetName() { return windowName; };
			inline void SetTextureSampler(VkSampler sampler) { textureSampler = sampler; };

		protected:

			virtual bool Begin(std::string name);
			void End();

			// Boolean for if window is currently visible
			bool show;

			// Name of window
			std::string windowName;

			ImGuiWindowFlags flags;

			// Vulkan Texture Sampler for Rendering Textures
			VkSampler textureSampler;
		};
	}
}