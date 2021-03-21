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

			UIWindowSceneHierarchy(Engine* InEngine, std::shared_ptr<ECS::World> InWorld) : UIWindow(InEngine, InWorld)
			{
			};

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

			inline bool HasEntityChanged() { return entityChanged; };
			inline ECS::Entity GetEntity() { return selectedEntity; };

		private:
			ECS::Entity selectedEntity;
			bool entityChanged;
		};
	}
}

#endif // !UI_WINDOW_SCENE_HIERARCHY_H