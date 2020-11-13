#pragma once
#include "UIWindow.h"
//#include "Entity.h"
#include "ECS.h"

namespace Puffin
{
    namespace UI
    {
        class UIWindowEntityProperties : public UIWindow
        {
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

            inline void SetEntity(ECS::Entity entity_) { entity = entity_; };
            inline void SetWorld(ECS::World* world_) { world = world_; };

        private:
            ECS::Entity entity;
            ECS::World* world;
        };
    }
}

