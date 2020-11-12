#pragma once

#include "UIWindow.h"
#include "UIWindowEntityProperties.h"
#include "EntitySystem.h"

namespace Puffin
{
	namespace UI
	{
		class UIWindowSceneHierarchy : public UIWindow
		{
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

			inline void SetEntitySystem(EntitySystem* entitySystem_) { entitySystem = entitySystem_; };
			inline void SetWindowProperties(UIWindowEntityProperties* windowProperties_) { windowProperties = windowProperties_; };

		private:

			EntitySystem* entitySystem;
			UIWindowEntityProperties* windowProperties;

			uint32_t selectedID;
			std::vector<uint32_t> entityIDs;
		};
	}
}