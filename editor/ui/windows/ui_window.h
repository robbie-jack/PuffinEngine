#pragma once

#include <imgui.h>

#include "input/input_subsystem.h"
#include "core/engine.h"
#include "types/uuid.h"

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

			explicit UIWindow(const std::shared_ptr<core::Engine>& engine);

			virtual ~UIWindow()
			{
				m_engine = nullptr;
			}

			virtual void Draw(double deltaTime) = 0;

			UUID GetSelectedEntity() const;
			void SetSelectedEntity(const UUID selectedEntity);

			bool GetShow() const;
			void Show();
			const std::string& GetName();

		protected:

			virtual bool Begin(const std::string& name);
			static void End();

			UUID mSelectedEntity = gInvalidID;

			// Boolean for if window is currently visible
			bool mShow;
			bool mFirstTime; // Flag to check if this is first time draw function was run

			// Name of window
			std::string mWindowName;

			ImGuiWindowFlags mFlags;

			std::shared_ptr<core::Engine> m_engine;

			
		};
	}
}
