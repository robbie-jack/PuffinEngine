#pragma once

#include "imgui.h"
#include "imfilebrowser.h"

#include "Core/Subsystem.h"
#include "UI/Editor/Windows/UIWindow.h"
#include "UI/Editor/Windows/UIWindowSceneHierarchy.h"
#include "UI/Editor/Windows/UIWindowViewport.h"
#include "UI/Editor/Windows/UIWindowSettings.h"
#include "UI/Editor/Windows/UIWindowEntityProperties.h"
#include "UI/Editor/Windows/UIWindowPerformance.h"
#include "UI/Editor/Windows/UIContentBrowser.h"

#include <vector>
#include <memory>

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace ui
	{
		enum class ImportAssetUI
		{
			None,
			Mesh,
			Texture
		};

		class UISubsystem final : public core::Subsystem
		{
		public:

			UISubsystem() {}
			~UISubsystem() override {}

			void setupCallbacks() override;

			void init();
			void render();
			void cleanup() const;

			void addWindow(const std::shared_ptr<UIWindow>& window);

			std::shared_ptr<UIWindowViewport> windowViewport() { return mWindowViewport; }
			std::shared_ptr<UIWindowSettings> windowSettings() { return mWindowSettings; }

		private:

			bool mSaveScene = false;
			bool mLoadScene = false;
			ImportAssetUI mImportAssetUI;

			PuffinID mEntity = gInvalidID;

			std::vector<std::shared_ptr<UIWindow>> mWindows;

			std::shared_ptr<UIWindowViewport> mWindowViewport;
			std::shared_ptr<UIWindowSettings> mWindowSettings;

			std::shared_ptr<UIWindowSceneHierarchy> mWindowSceneHierarchy;
			
			
			std::shared_ptr<UIWindowEntityProperties> mWindowEntityProperties;
			std::shared_ptr<UIWindowPerformance> mWindowPerformance;
			std::shared_ptr<UIContentBrowser> mContentBrowser;

			ImGui::FileBrowser mFileDialog;
			std::string mImGuiIniFilename;

			void showDockspace(bool* open);
			void showMenuBar();
			void setStyle();
		};
	}
}