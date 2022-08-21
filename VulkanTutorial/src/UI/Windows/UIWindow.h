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
		typedef uint32_t EntityID;
	}

	namespace Core
	{
		class Engine;
	}

	namespace UI
	{
		class UIWindow
		{
		public:

			UIWindow(Core::Engine* InEngine, std::shared_ptr<ECS::World> InWorld, std::shared_ptr<Input::InputManager> InInput)
				: m_engine(InEngine), m_world(InWorld), m_inputManager(InInput)
			{
				show = true;
				firstTime = true;
				flags = ImGuiWindowFlags_None;
			}

			virtual ~UIWindow()
			{
				m_world = nullptr;
				m_engine = nullptr;
				m_inputManager = nullptr;
			}

			virtual void Draw(float dt) = 0;

			void Show();

			inline bool* GetShow() { return &show; }
			inline std::string GetName() { return windowName; }
			inline void SetTextureSampler(VkSampler sampler) { textureSampler = sampler; }

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

			Core::Engine* m_engine;
			std::shared_ptr<ECS::World> m_world;
			std::shared_ptr<Input::InputManager> m_inputManager;
		};
	}
}

#endif // UI_WINDOW_H