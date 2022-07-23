#pragma once

#include "UIWindow.h"
#include "UIWindowEntityProperties.h"

#include "ECS/ECS.h"

namespace Puffin
{
	namespace UI
	{
		class UIWindowSceneHierarchy : public UIWindow
		{
		public:

			UIWindowSceneHierarchy(Engine* InEngine, std::shared_ptr<ECS::World> InWorld, std::shared_ptr<Input::InputManager> InInput)
				: UIWindow(InEngine, InWorld, InInput)
			{
			};

			void Draw(float dt) override;

			inline bool HasEntityChanged() { return entityChanged; };
			inline ECS::Entity GetEntity() { return selectedEntity; };

		private:
			ECS::Entity selectedEntity;
			bool entityChanged;
		};
	}
}