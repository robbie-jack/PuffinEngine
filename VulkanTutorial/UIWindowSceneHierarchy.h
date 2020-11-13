#pragma once

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

			inline void SetEntityManager(ECS::EntityManager* entityManager_) { entityManager = entityManager_; };
			inline void SetWindowProperties(UIWindowEntityProperties* windowProperties_) { windowProperties = windowProperties_; };

		private:

			ECS::EntityManager* entityManager;
			UIWindowEntityProperties* windowProperties;

			ECS::Entity selectedEntity;
			//std::vector<uint32_t> entityIDs;
		};
	}
}