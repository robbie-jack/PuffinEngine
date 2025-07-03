#pragma once

#include "imgui.h"
#include "imfilebrowser.h"

#include <vector>
#include <memory>

#include "subsystem/editor_subsystem.h"
#include "types/uuid.h"

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
			Default,
			Mesh,
			Texture
		};

		class EditorUISubsystem : public core::EditorSubsystem
		{
		public:

			explicit EditorUISubsystem(const std::shared_ptr<core::Engine>& engine);
			~EditorUISubsystem() override = default;

			void Initialize(core::SubsystemManager* subsystemManager) override;
			void Deinitialize() override;

			void Update(double deltaTime) override;
			bool ShouldUpdate() override;

			std::string_view GetName() const override;

			void AddWindow(const std::shared_ptr<UIWindow>& window);

			std::shared_ptr<UIWindowViewport> GetWindowViewport();
			std::shared_ptr<UIWindowSettings> GetWindowSettings();

		private:

			void ShowDockspace(bool* open);
			void ShowMenuBar();
			void SetStyle();

			bool mSaveScene = false;
			bool mLoadScene = false;
			ImportAssetUI mImportAssetUI = ImportAssetUI::Default;

			UUID mEntity = gInvalidID;

			std::vector<std::shared_ptr<UIWindow>> mWindows;

			std::shared_ptr<UIWindowViewport> mWindowViewport;
			std::shared_ptr<UIWindowSettings> mWindowSettings;
			std::shared_ptr<UIWindowSceneHierarchy> mWindowSceneHierarchy;
			std::shared_ptr<UIWindowNodeEditor> mWindowNodeEditor;
			std::shared_ptr<UIWindowPerformance> mWindowPerformance;
			std::shared_ptr<UIContentBrowser> mContentBrowser;

			ImGui::FileBrowser mFileDialog;
			std::string mImguiIniFilename;
			
		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<ui::EditorUISubsystem>()
		{
			return "EditorUISubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<ui::EditorUISubsystem>()
		{
			return entt::hs(GetTypeString<ui::EditorUISubsystem>().data());
		}

		template<>
		inline void RegisterType<ui::EditorUISubsystem>()
		{
			auto meta = entt::meta<ui::EditorUISubsystem>()
				.base<core::EditorSubsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}
