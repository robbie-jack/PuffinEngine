#pragma once

#include <imgui.h>

#include "puffin/input/inputsubsystem.h"
#include "puffin/core/engine.h"
#include "puffin/types/uuid.h"

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

			UIWindow(const std::shared_ptr<core::Engine>& engine) : m_engine(engine)
			{
				mShow = true;
				mFirstTime = true;
				mFlags = ImGuiWindowFlags_None;
			}

			virtual ~UIWindow()
			{
				m_engine = nullptr;
			}

			virtual void draw(double dt) = 0;

			UUID selectedEntity() const { return mSelectedEntity; }
			void setSelectedEntity(const UUID selectedEntity) { mSelectedEntity = selectedEntity; }

			void setShow();

			bool* show() { return &mShow; }
			std::string name() { return mWindowName; }

		protected:

			UUID mSelectedEntity = gInvalidId;

			// Boolean for if window is currently visible
			bool mShow;
			bool mFirstTime; // Flag to check if this is first time draw function was run

			// Name of window
			std::string mWindowName;

			ImGuiWindowFlags mFlags;

			std::shared_ptr<core::Engine> m_engine;

			virtual bool begin(std::string name);
			static void end();
		};
	}
}
