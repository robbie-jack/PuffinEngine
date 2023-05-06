#pragma once

#include "UIWindow.h"
#include "UIWindowEntityProperties.h"
#include "Types/UUID.h"

namespace puffin
{
	namespace ui
	{
		class UIWindowSceneHierarchy : public UIWindow
		{
		public:

			UIWindowSceneHierarchy(const std::shared_ptr<core::Engine>& engine) : UIWindow(engine) {}
			~UIWindowSceneHierarchy() override {}

			void draw(double dt) override;

			[[nodiscard]] bool entityChanged() const { return mEntityChanged; }

		private:
			
			bool mEntityChanged = false;

		};
	}
}
