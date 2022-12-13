#include "UI/Editor/Windows/UIContentBrowser.h"

#include "Assets/AssetRegistry.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace Puffin::UI
{
	void UIContentBrowser::Draw(double dt)
	{
		if (show)
		{
			ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

			Begin(windowName);

			fs::path contentPath = Assets::AssetRegistry::Get()->ContentRoot();
			for (auto const& dir_entry : fs::directory_iterator(contentPath))
			{
				std::string pathName = dir_entry.path().string();
				ImGui::Text(pathName.c_str());
			}

			End();
		}
	}
}
