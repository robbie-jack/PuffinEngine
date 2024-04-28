#pragma once

#include "puffin/ui/editor/windows/ui_window.h"

namespace puffin
{
	namespace scene
	{
		class SceneGraph;
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

			[[nodiscard]] bool entityChanged() const { return mEntityChanged; }

		private:
			
			bool mEntityChanged = false;

			std::shared_ptr<scene::SceneGraph> m_scene_graph = nullptr;

			void draw_node_ui(PuffinID id, const ImGuiTreeNodeFlags& base_flags);

		};
	}
}
