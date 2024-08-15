#include "puffin/editor/ui/windows/uiwindowcontentbrowser.h"

#include <filesystem>

#include "puffin/assets/assetregistry.h"

namespace fs = std::filesystem;

namespace puffin::ui
{
	void UIContentBrowser::draw(double dt)
	{
		if (mShow)
		{
			ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

			begin(mWindowName);

			ImVec2 listBoxSize = ImGui::GetWindowSize();
			listBoxSize.y -= 20.0f;

			const fs::path contentPath = assets::AssetRegistry::get()->content_root();

			if (exists(contentPath))
			{
				if (ImGui::BeginListBox("##ContentList", listBoxSize))
				{
					constexpr ImGuiTreeNodeFlags baseFlags = ImGuiTreeNodeFlags_OpenOnArrow
						| ImGuiTreeNodeFlags_OpenOnDoubleClick
						| ImGuiTreeNodeFlags_SpanAvailWidth;

					mNodeCounter = 0;

					ContentEntryNode rootNode;
					rootNode.index = mNodeCounter;

					mNodeCounter++;

					GenerateChildEntryNodes(fs::directory_entry(contentPath), rootNode);

					for (const auto& node : rootNode.childNodes)
					{
						GenerateUIForNode(node, baseFlags);
					}

					ImGui::EndListBox();
				}
			}

			end();
		}
	}

	void UIContentBrowser::GenerateChildEntryNodes(const fs::directory_entry& dirEntry, ContentEntryNode& rootNode)
	{
		fs::path path = dirEntry.path();

		for (auto const& entry : fs::directory_iterator(path))
		{
			ContentEntryNode node;
			node.displayName = entry.path().filename().string();
			node.index = mNodeCounter;

			mNodeCounter++;

			if (entry.is_directory())
			{
				node.entryType = EntryType::Directory;

				GenerateChildEntryNodes(entry, node);
			}
			else if (entry.is_regular_file())
			{
				node.entryType = EntryType::File;
			}

			rootNode.childNodes.push_back(node);
		}
	}

	void UIContentBrowser::GenerateUIForNode(const ContentEntryNode& node, const ImGuiTreeNodeFlags& baseFlags)
	{
		const ImGuiTreeNodeFlags treeFlags = baseFlags;

		switch(node.entryType)
		{
		case EntryType::Directory:

			if (ImGui::TreeNodeEx(node.displayName.c_str(), treeFlags))
			{
				for (const auto& childNode : node.childNodes)
				{
					GenerateUIForNode(childNode, baseFlags);
				}

				ImGui::TreePop();
			}

			break;
		case EntryType::File:

			int idx = node.index;

			ImGui::Dummy(ImVec2(10.0f, 0.0f)); ImGui::SameLine();
			if (ImGui::Selectable(node.displayName.c_str(), mSelectedNode == idx))
			{
				mSelectedNode = idx;
			}
			if (ImGui::BeginPopupContextItem(node.displayName.c_str()))
			{
				mSelectedNode = idx;

				ImGui::Text("Popup for %s", node.displayName.c_str());
				ImGui::EndPopup();
			}

			break;
		}
	}
}
