#pragma once

#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include <string>
#include <vulkan/vulkan.h>
#include <imgui.h>

#include <Input\InputManager.h>
#include <ECS/ECS.h>
#include <Engine.h>

namespace Puffin
{
	namespace ECS
	{
		class World;
		typedef uint32_t Entity;
	}
	
	class Engine;

	namespace UI
	{
		class UIWindow
		{
		public:

			UIWindow(Engine* InEngine, ECS::World* InWorld);
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
			bool firstTime; // Flag to check if this is first time draw function was run

			// Name of window
			std::string windowName;

			ImGuiWindowFlags flags;

			// Vulkan Texture Sampler for Rendering Textures
			VkSampler textureSampler;

			Engine* engine;
			ECS::World* world;
		};
	}
}

#endif // UI_WINDOW_H