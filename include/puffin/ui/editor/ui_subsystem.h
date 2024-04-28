#pragma once

#include "imgui.h"
#include "imfilebrowser.h"

#include "puffin/core/system.h"

#include <vector>
#include <memory>

#include "puffin/types/uuid.h"

namespace puffin
{
	namespace core
	{
		class Engine;
	}

	namespace ui
	{
		class UIContentBrowser;
		class UIWindowPerformance;
		class UIWindowNodeEditor;
		class UIWindowSceneHierarchy;
		class UIWindowSettings;
		class UIWindowViewport;
		class UIWindow;

		enum class ImportAssetUI
		{
			None,
			Mesh,
			Texture
		};

		class UISubsystem : public core::System
		{
		public:

			UISubsystem(const std::shared_ptr<core::Engine>& engine);
			~UISubsystem() override { mEngine = nullptr; }

			void startup();
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
			
			
			std::shared_ptr<UIWindowNodeEditor> mWindowEntityProperties;
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
