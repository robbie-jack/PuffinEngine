#pragma once
#include "UIWindow.h"
#include "Entity.h"

namespace Puffin
{
    namespace UI
    {
        class UIWindowEntityProperties : public UIWindow
        {
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

            inline void SetEntity(Entity* entity_) { entity = entity_; };

        private:
            Entity* entity;
        };
    }
}

