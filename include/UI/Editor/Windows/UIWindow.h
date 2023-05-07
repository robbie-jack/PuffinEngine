#pragma once

#include <imgui.h>

#include "Input/InputSubsystem.h"
#include "Core/Engine.h"
#include "Types/UUID.h"

#include <string>

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace ui
	{
		class UIWindow
		{
		public:

			UIWindow(const std::shared_ptr<core::Engine>& engine) : mEngine(engine)
			{
				mShow = true;
				mFirstTime = true;
				mFlags = ImGuiWindowFlags_None;
			}

			virtual ~UIWindow()
			{
				mEngine = nullptr;
			}

			virtual void draw(double dt) = 0;

			PuffinID selectedEntity() const { return mSelectedEntity; }
			void setSelectedEntity(const PuffinID selectedEntity) { mSelectedEntity = selectedEntity; }

			void setShow();

			bool* show() { return &mShow; }
			std::string name() { return mWindowName; }

		protected:

			PuffinID mSelectedEntity;

			// Boolean for if window is currently visible
			bool mShow;
			bool mFirstTime; // Flag to check if this is first time draw function was run

			// Name of window
			std::string mWindowName;

			ImGuiWindowFlags mFlags;

			std::shared_ptr<core::Engine> mEngine;

			virtual bool begin(std::string name);
			static void end();
		};
	}
}
