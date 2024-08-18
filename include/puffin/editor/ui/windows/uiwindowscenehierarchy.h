#pragma once

#include "puffin/editor/ui/windows/uiwindow.h"

namespace puffin
{
	namespace scene
	{
		class SceneGraphSubsystem;
	}
}

namespace puffin
{
	namespace ui
	{
		class UIWindowSceneHierarchy : public UIWindow
		{
		public:

			explicit UIWindowSceneHierarchy(const std::shared_ptr<core::Engine>& engine);
			~UIWindowSceneHierarchy() override = default;

			void Draw(double deltaTime) override;

			[[nodiscard]] bool GetEntityChanged() const;

		private:

			void DrawNodeUI(UUID id, const ImGuiTreeNodeFlags& baseFlags);

			bool mEntityChanged = false;

			

		};
	}
}
