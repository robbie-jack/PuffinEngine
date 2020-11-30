#pragma once

#ifndef UI_WINDOW_SCENE_HIERARCHY_H
#define UI_WINDOW_SCENE_HIERARCHY_H

#include "UIWindow.h"
#include "UIWindowEntityProperties.h"
//#include "EntitySystem.h"
#include "ECS.h"

namespace Puffin
{
	namespace UI
	{
		class UIWindowSceneHierarchy : public UIWindow
		{
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

			inline void SetWorld(ECS::World* world_) { world = world_; };
			inline void SetWindowProperties(UIWindowEntityProperties* windowProperties_) { windowProperties = windowProperties_; };

		private:

			ECS::World* world;
			UIWindowEntityProperties* windowProperties;

			ECS::Entity selectedEntity;
		};
	}
}

#endif // !UI_WINDOW_SCENE_HIERARCHY_H