#pragma once

#include "UIWindow.h"
#include "UIWindowEntityProperties.h"

#include "ECS/ECS.h"

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

			inline bool HasEntityChanged() { return entityChanged; };
			inline ECS::EntityID GetEntity() { return selectedEntity; };

		private:
			ECS::EntityID selectedEntity = ECS::INVALID_ENTITY;
			bool entityChanged = false;
		};
	}
}