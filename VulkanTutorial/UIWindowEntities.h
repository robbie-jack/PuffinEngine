#pragma once

#include "UIWindow.h"
#include "EntitySystem.h"

namespace Puffin
{
	namespace UI
	{
		class UIWindowEntities : public UIWindow
		{
		public:

			bool Draw(float dt, Puffin::Input::InputManager* InputManager) override;

			void SetEntityIDs(std::vector<uint32_t> ids);

		private:

			EntitySystem* entitySystem;
			std::vector<uint32_t> entityIDs;
		};
	}
}