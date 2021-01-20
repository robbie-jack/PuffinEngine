#pragma once

#ifndef UI_WINDOW_SCENE_HIERARCHY_H
#define UI_WINDOW_SCENE_HIERARCHY_H

#include <UI/UIWindow.h>
#include <UI/UIWindowEntityProperties.h>
#include <ECS/ECS.h>

namespace Puffin
{
	namespace UI
	{
		class UIWindowSceneHierarchy : public UIWindow
		{
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

			inline void SetWorld(ECS::World* world_) { world = world_; };

			inline bool HasEntityChanged() { return entityChanged; };
			inline ECS::Entity GetEntity() { return selectedEntity; };

		private:

			ECS::World* world;
			UIWindowEntityProperties* windowProperties;

			ECS::Entity selectedEntity;

			bool entityChanged;
		};
	}
}

#endif // !UI_WINDOW_SCENE_HIERARCHY_H