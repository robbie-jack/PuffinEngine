#pragma once

#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include <string>
#include <vulkan/vulkan.h>
#include <imgui.h>

#include <Input\InputSubsystem.h>
#include <ECS/ECS.h>
#include "Engine/Engine.hpp"

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

			UIWindow(std::shared_ptr<Core::Engine> engine) : m_engine(engine)
			{
				show = true;
				firstTime = true;
				flags = ImGuiWindowFlags_None;
			}

			virtual ~UIWindow()
			{
				m_engine = nullptr;
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

			std::shared_ptr<Core::Engine> m_engine;
		};
	}
}

#endif // UI_WINDOW_H