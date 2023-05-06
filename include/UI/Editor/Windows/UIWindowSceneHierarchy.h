#pragma once

#include "UIWindow.h"
#include "UIWindowEntityProperties.h"
#include "Types/UUID.h"

namespace puffin
{
	namespace UI
	{
		class UIWindowSceneHierarchy : public UIWindow
		{
		public:

			UIWindowSceneHierarchy(std::shared_ptr<core::Engine> engine) : UIWindow(engine) {}
			~UIWindowSceneHierarchy() override {}

			void Draw(double dt) override;

			bool HasEntityChanged() { return entityChanged; }
			PuffinId GetEntity() { return selectedEntity; }

		private:
			PuffinId selectedEntity;
			bool entityChanged = false;
		};
	}
}
