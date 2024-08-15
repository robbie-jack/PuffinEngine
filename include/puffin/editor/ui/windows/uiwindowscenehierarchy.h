#pragma once

#include "puffin/ui/editor/windows/ui_window.h"

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

			UIWindowSceneHierarchy(const std::shared_ptr<core::Engine>& engine) : UIWindow(engine) {}
			~UIWindowSceneHierarchy() override {}

			void draw(double dt) override;

			[[nodiscard]] bool entityChanged() const { return m_entity_changed; }

		private:
			
			bool m_entity_changed = false;

			void draw_node_ui(PuffinID id, const ImGuiTreeNodeFlags& base_flags);

		};
	}
}
