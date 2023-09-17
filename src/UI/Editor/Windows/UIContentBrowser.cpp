#include "UI/Editor/Windows/UIContentBrowser.h"

#include "Assets/AssetRegistry.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace puffin::ui
{
	void UIContentBrowser::draw(double dt)
	{
		if (mShow)
		{
			ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

			begin(mWindowName);

			const fs::path contentPath = assets::AssetRegistry::get()->contentRoot();
			for (auto const& dirEntry : fs::directory_iterator(contentPath))
			{
				std::string pathName = dirEntry.path().string();
                ImGui::Text("%s", pathName.c_str());
			}

			end();
		}
	}
}
