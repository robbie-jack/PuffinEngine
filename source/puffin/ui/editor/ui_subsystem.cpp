#include "puffin/ui/editor/ui_subsystem.h"

#include <string>
#include <iostream>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "puffin/core/engine.h"
#include "puffin/core/scene_subsystem.h"
#include "puffin/assets/asset_importers.h"
#include "puffin/assets/asset_registry.h"
#include "puffin/ui/editor/windows/ui_window.h"
#include "puffin/ui/editor/windows/ui_content_browser.h"
#include "puffin/ui/editor/windows/ui_window_node_editor.h"
#include "puffin/ui/editor/windows/ui_window_performance.h"
#include "puffin/ui/editor/windows/ui_window_scene_hierarchy.h"
#include "puffin/ui/editor/windows/ui_window_settings.h"
#include "puffin/ui/editor/windows/ui_window_viewport.h"

namespace fs = std::filesystem;

namespace puffin::ui
{
	UISubsystem::UISubsystem(const std::shared_ptr<core::Engine>& engine) : System(engine)
	{
		m_engine->register_callback(core::ExecutionStage::StartupSubsystem, [&]() { startup(); }, "UISubsystem: startup", 50);
		m_engine->register_callback(core::ExecutionStage::Render, [&]() { render(); }, "UISubsystem: render", 50);
		m_engine->register_callback(core::ExecutionStage::ShutdownSubsystem, [&]() { cleanup(); }, "UISubsystem: cleanup", 200);
	}

	void UISubsystem::startup()
	{
		if (m_engine->should_render_editor_ui())
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			//ImPlot::CreateContext();

			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

			mImGuiIniFilename = (assets::AssetRegistry::get()->engineRoot() / "config" / "imgui.ini").string();
			io.IniFilename = mImGuiIniFilename.c_str();

			setStyle();

			mSaveScene = false;
			mLoadScene = false;
			mImportAssetUI = ImportAssetUI::None;

			mWindowSceneHierarchy = std::make_shared<UIWindowSceneHierarchy>(m_engine);
			mWindowViewport = std::make_shared<UIWindowViewport>(m_engine);
			mWindowSettings = std::make_shared<UIWindowSettings>(m_engine);
			mWindowEntityProperties = std::make_shared<UIWindowNodeEditor>(m_engine);
			mWindowPerformance = std::make_shared<UIWindowPerformance>(m_engine);
			mContentBrowser = std::make_shared<UIContentBrowser>(m_engine);

			mWindowEntityProperties->set_file_browser(&mFileDialog);

			addWindow(mWindowSceneHierarchy);
			addWindow(mWindowSettings);
			addWindow(mWindowEntityProperties);
			addWindow(mWindowPerformance);
			addWindow(mContentBrowser);
		}
	}

	// Any functionality which would cause errors/crashes from within 
	// VulkanRenderer DrawFrame() should be placed here instead
	void UISubsystem::render()
	{
		if (m_engine->should_render_editor_ui())
		{
			if (mFileDialog.HasSelected())
			{
				const fs::path selectedPath = mFileDialog.GetSelected();

				// File Dialog - Load Scene
				if (mLoadScene)
				{
					const auto sceneData = m_engine->get_system<io::SceneSubsystem>()->sceneData();

					sceneData->set_path(selectedPath);

					sceneData->load();

					m_engine->restart();

					mLoadScene = false;
				}

				switch (mImportAssetUI)
				{
				// File Dialog - Import Mesh
				case ImportAssetUI::Mesh:

					if (io::load_and_import_model(selectedPath, selectedPath.parent_path().stem()))
					{
						std::cout << "Import Successful" << std::endl;
					}
					else
					{
						std::cout << "Import Failed" << std::endl;
					}

					mImportAssetUI = ImportAssetUI::None;

					break;

				// File Dialog - Import Texture
				case ImportAssetUI::Texture:
						
					if (io::load_and_import_texture(selectedPath, selectedPath.parent_path().stem()))
					{
						std::cout << "Import Successful" << std::endl;
					}
					else
					{
						std::cout << "Import Failed" << std::endl;
					}

					mImportAssetUI = ImportAssetUI::None;

					break;

				default: ;
				}
			}

			// Update Selected Entity
			if (mWindowSceneHierarchy->entityChanged())
			{
				mEntity = mWindowSceneHierarchy->selectedEntity();

				for (const auto& window : mWindows)
				{
					window->setSelectedEntity(mEntity);
				}
			}

			// Update Scene Data if any changes were made to an entity, and game is not currently playing
			if (mWindowEntityProperties->scene_changed() && m_engine->play_state() == core::PlayState::Stopped)
			{
				const auto sceneData = m_engine->get_system<io::SceneSubsystem>()->sceneData();
				sceneData->update_data(m_engine->get_system<ecs::EnTTSubsystem>(), m_engine->get_system<scene::SceneGraph>());
			}

			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			bool* pOpen = nullptr;

			// Show Dockspace with Menu Bar for Editor Windows
			showDockspace(pOpen);

			//ImGui::ShowDemoWindow(p_open);
			//ImPlot::ShowDemoWindow(p_open);

			// Draw UI Windows
			if (!mWindows.empty())
			{
				for (const auto& window : mWindows)
				{
					window->draw(m_engine->delta_time());
				}
			}

			mFileDialog.Display();
		}
	}

	void UISubsystem::cleanup() const
	{
		if (m_engine->should_render_editor_ui())
		{
			ImGui::DestroyContext();
			//ImPlot::DestroyContext();
		}
	}

	void UISubsystem::showDockspace(bool* open)
	{
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", open, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		showMenuBar();

		if (mSaveScene)
		{
			mSaveScene = false;
			ImGui::OpenPopup("Save Scene");
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5, 0.5));

		// Save Scene Modal Window
		if (ImGui::BeginPopupModal("Save Scene", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			const auto sceneData = m_engine->get_system<io::SceneSubsystem>()->sceneData();

			std::string str_name = sceneData->path().string();
			std::vector<char> name(256, '\0');
			for (int i = 0; i < str_name.size(); i++)
			{
				name[i] = str_name[i];
			}
			name.push_back('\0');

			ImGui::Text("Enter Scene Name:");
			if (ImGui::InputText("##Edit", &name[0], name.size(), ImGuiInputTextFlags_EnterReturnsTrue))
			{
				sceneData->set_path(std::string(&name[0]));
			}

			if (ImGui::Button("Save"))
			{
				const auto entt_subsystem = m_engine->get_system<ecs::EnTTSubsystem>();
				const auto scene_graph = m_engine->get_system<scene::SceneGraph>();

				sceneData->update_data(entt_subsystem, scene_graph);
				sceneData->save();

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void UISubsystem::showMenuBar()
	{
		if (ImGui::BeginMenuBar())
		{
			// File Options
			if (ImGui::BeginMenu("File"))
			{
				// Project Options
				ImGui::Text("---Project---");

				if (ImGui::MenuItem("New Project"))
				{

				}

				if (ImGui::MenuItem("Load Project"))
				{

				}

				if (ImGui::MenuItem("Save Project"))
				{
					io::save_settings(assets::AssetRegistry::get()->projectRoot() / "settings.json", m_engine->settings());
					assets::AssetRegistry::get()->saveAssetCache();
				}

				if (ImGui::MenuItem("Save Project As"))
				{

				}

				// Scene Options
				ImGui::Text("---Scene---");

				if (ImGui::MenuItem("New Scene"))
				{

				}

				if (ImGui::MenuItem("Load Scene"))
				{
					mFileDialog.Open();
					mLoadScene = true;
				}

				if (ImGui::MenuItem("Save Scene"))
				{
					const auto sceneData = m_engine->get_system<io::SceneSubsystem>()->sceneData();

					const auto entt_subsystem = m_engine->get_system<ecs::EnTTSubsystem>();
					const auto scene_graph = m_engine->get_system<scene::SceneGraph>();

					sceneData->update_data(entt_subsystem, scene_graph);
					sceneData->save();
				}

				if (ImGui::MenuItem("Save Scene As"))
				{
					mSaveScene = true;
				}

				ImGui::Text("---Other---");

				if (ImGui::BeginMenu("Import"))
				{
					if (ImGui::MenuItem("Mesh"))
					{
						mFileDialog.Open();
						mImportAssetUI = ImportAssetUI::Mesh;
					}

					if (ImGui::MenuItem("Texture"))
					{
						mFileDialog.Open();
						mImportAssetUI = ImportAssetUI::Texture;
					}

					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Quit", "Alt+F4"))
				{
					m_engine->exit();
				}

				ImGui::EndMenu();
			}

			// List all windows
			if (ImGui::BeginMenu("Windows"))
			{
				if (mWindows.size() > 0)
				{
					for (int i = 0; i < mWindows.size(); i++)
					{
						// Show/Hide window if clicked
						ImGui::MenuItem(mWindows[i]->name().c_str(), NULL, mWindows[i]->show());
					}
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
	}

	void UISubsystem::setStyle()
	{
		ImGuiStyle* style = &ImGui::GetStyle();
		ImVec4* colors = style->Colors;

		colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.000f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.000f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.000f);
		colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.000f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.000f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.000f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.6f, 0.0f, 0.156f);
		colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
		colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.000f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
		colors[ImGuiCol_SeparatorHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.000f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_Tab] = ImVec4(0.05f, 0.05f, 0.05f, 1.000f);
		colors[ImGuiCol_TabHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.000f);
		colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.000f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.000f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

		style->ChildRounding = 4.0f;
		style->FrameBorderSize = 1.0f;
		style->FrameRounding = 2.0f;
		style->GrabMinSize = 7.0f;
		style->PopupRounding = 2.0f;
		style->ScrollbarRounding = 12.0f;
		style->ScrollbarSize = 13.0f;
		style->TabBorderSize = 1.0f;
		style->TabRounding = 0.0f;
		style->WindowRounding = 4.0f;
		style->WindowPadding = { 0.0f, 0.0f };
	}

	void UISubsystem::addWindow(const std::shared_ptr<UIWindow>& window)
	{
		mWindows.push_back(window);
	}
}