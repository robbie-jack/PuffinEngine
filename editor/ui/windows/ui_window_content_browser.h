#pragma once

#include "ui/windows/ui_window.h"

namespace puffin::ui
{
	class UIContentBrowser : public UIWindow
	{
	public:
			
		explicit UIContentBrowser(const std::shared_ptr<core::Engine>& engine);
		~UIContentBrowser() override = default;

		void Draw(double deltaTime) override;

	private:

		enum class EntryType
		{
			Directory = 0,
			File = 1
		};

		struct ContentEntryNode
		{
			std::string displayName;
			EntryType entryType = EntryType::Directory;
			int index = 0;

			std::vector<ContentEntryNode> childNodes;
		};

		void GenerateChildEntryNodes(const fs::directory_entry& dirEntry, ContentEntryNode& rootNode);
		void GenerateUIForNode(const ContentEntryNode& node, const ImGuiTreeNodeFlags& baseFlags);

		int mNodeCounter = 0;
		int mSelectedNode = 0;

	};
}
