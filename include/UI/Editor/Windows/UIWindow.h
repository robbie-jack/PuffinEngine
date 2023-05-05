#pragma once

#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include <string>
#include <vulkan/vulkan.h>
#include <imgui.h>

#include <Input\InputSubsystem.h>
#include <ECS/ECS.h>
#include "Engine\Engine.h"

namespace puffin
{
	namespace ECS
	{
		class World;
	}

	namespace core
	{
		class Engine;
	}

	namespace UI
	{
		class UIWindow
		{
		public:

			UIWindow(std::shared_ptr<core::Engine> engine) : m_engine(engine)
			{
				show = true;
				firstTime = true;
				flags = ImGuiWindowFlags_None;
			}

			virtual ~UIWindow()
			{
				m_engine = nullptr;
			}

			virtual void Draw(double dt) = 0;

			void Show();

			inline bool* GetShow() { return &show; }
			inline std::string GetName() { return windowName; }

		protected:

			virtual bool Begin(std::string name);
			void End();

			// Boolean for if window is currently visible
			bool show;
			bool firstTime; // Flag to check if this is first time draw function was run

			// Name of window
			std::string windowName;

			ImGuiWindowFlags flags;

			std::shared_ptr<core::Engine> m_engine;
		};
	}
}

#endif // UI_WINDOW_H