#pragma once

#include "UIWindow.h"

namespace puffin::ui
{
	class UIContentBrowser : public UIWindow
	{
	public:
			
		UIContentBrowser(const std::shared_ptr<core::Engine>& engine) : UIWindow(engine)
		{
			mWindowName = "Content Browser";
		}
		~UIContentBrowser() override {}

		void draw(double dt) override;


	private:

		enum class EntryType
		{
			Directory = 0,
			File = 1
		};

		struct ContentEntryNode
		{
			std::string displayName;
			EntryType entryType;
			int index = 0;

			std::vector<ContentEntryNode> childNodes;
		};

		int mNodeCounter = 0;
		int mSelectedNode = 0;

		void GenerateChildEntryNodes(const fs::directory_entry& dirEntry, ContentEntryNode& rootNode);
		void GenerateUIForNode(const ContentEntryNode& node, const ImGuiTreeNodeFlags& baseFlags);

	};
}