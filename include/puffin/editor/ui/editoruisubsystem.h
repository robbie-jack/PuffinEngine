#pragma once

#include "imgui.h"
#include "imfilebrowser.h"

#include <vector>
#include <memory>

#include "puffin/core/subsystem.h"
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
			Default,
			Mesh,
			Texture
		};

		class EditorUISubsystem : public core::Subsystem
		{
		public:

			explicit EditorUISubsystem(const std::shared_ptr<core::Engine>& engine);
			~EditorUISubsystem() override = default;

			void Initialize(core::SubsystemManager* subsystem_manager) override;
			void Deinitialize() override;

			void Update(double delta_time) override;
			bool ShouldUpdate() override;

			void add_window(const std::shared_ptr<UIWindow>& window);

			std::shared_ptr<UIWindowViewport> window_viewport();
			std::shared_ptr<UIWindowSettings> window_settings();

		private:

			bool m_save_scene = false;
			bool m_load_scene = false;
			ImportAssetUI m_import_asset_ui;

			UUID m_entity = gInvalidId;

			std::vector<std::shared_ptr<UIWindow>> m_windows;

			std::shared_ptr<UIWindowViewport> m_window_viewport;
			std::shared_ptr<UIWindowSettings> m_window_settings;
			std::shared_ptr<UIWindowSceneHierarchy> m_window_scene_hierarchy;
			std::shared_ptr<UIWindowNodeEditor> m_window_entity_properties;
			std::shared_ptr<UIWindowPerformance> m_window_performance;
			std::shared_ptr<UIContentBrowser> m_content_browser;

			ImGui::FileBrowser m_file_dialog;
			std::string m_imgui_ini_filename;

			void show_dockspace(bool* open);
			void show_menu_bar();
			void set_style();
		};
	}
}
