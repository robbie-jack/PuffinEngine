#include "puffin/editor/ui/editoruisubsystem.h"

#include <string>
#include <iostream>

//#include "backends/imgui_impl_glfw.h"
//#include "backends/imgui_impl_vulkan.h"

#include "puffin/core/engine.h"
#include "puffin/scene/sceneserializationsubsystem.h"
#include "puffin/assets/assetimporters.h"
#include "puffin/assets/assetregistry.h"
#include "puffin/editor/ui/windows/uiwindow.h"
#include "puffin/editor/ui/windows/uiwindowcontentbrowser.h"
#include "puffin/editor/ui/windows/uiwindownodeeditor.h"
#include "puffin/editor/ui/windows/uiwindowperformance.h"
#include "puffin/editor/ui/windows/uiwindowscenehierarchy.h"
#include "puffin/editor/ui/windows/uiwindowsettings.h"
#include "puffin/editor/ui/windows/uiwindowviewport.h"
#include "puffin/core/settingsmanager.h"

namespace fs = std::filesystem;

namespace puffin::ui
{
	EditorUISubsystem::EditorUISubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "EditorUISubsystem";
	}

	void EditorUISubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		//ImPlot::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		mImguiIniFilename = (assets::AssetRegistry::Get()->GetEngineRoot() / "config" / "imgui.ini").string();
		io.IniFilename = mImguiIniFilename.c_str();

		SetStyle();

		mSaveScene = false;
		mLoadScene = false;
		mImportAssetUI = ImportAssetUI::Default;

		mWindowSceneHierarchy = std::make_shared<UIWindowSceneHierarchy>(mEngine);
		mWindowViewport = std::make_shared<UIWindowViewport>(mEngine);
		mWindowSettings = std::make_shared<UIWindowSettings>(mEngine);
		mWindowNodeEditor = std::make_shared<UIWindowNodeEditor>(mEngine);
		mWindowPerformance = std::make_shared<UIWindowPerformance>(mEngine);
		mContentBrowser = std::make_shared<UIContentBrowser>(mEngine);

		mWindowNodeEditor->SetFileBrowser(&mFileDialog);

		AddWindow(mWindowSceneHierarchy);
		AddWindow(mWindowSettings);
		AddWindow(mWindowNodeEditor);
		AddWindow(mWindowPerformance);
		AddWindow(mContentBrowser);
	}

	void EditorUISubsystem::Deinitialize()
	{
		ImGui::DestroyContext();
		//ImPlot::DestroyContext();
	}

	void EditorUISubsystem::Update(double deltaTime)
	{
		if (mFileDialog.HasSelected())
		{
			const fs::path selectedPath = mFileDialog.GetSelected();

			// File Dialog - Load Scene
			if (mLoadScene)
			{
				const auto sceneData = mEngine->GetSubsystem<io::SceneSerializationSubsystem>()->GetSceneData();

				sceneData->SetPath(selectedPath);

				sceneData->Load();

				mEngine->Restart();

				mLoadScene = false;
			}

			switch (mImportAssetUI)
			{
				// File Dialog - Import Mesh
			case ImportAssetUI::Mesh:

				if (io::LoadAndImportModel(selectedPath, selectedPath.parent_path().stem()))
				{
					std::cout << "Import Successful" << std::endl;
				}
				else
				{
					std::cout << "Import Failed" << std::endl;
				}

				mImportAssetUI = ImportAssetUI::Default;

				break;

				// File Dialog - Import Texture
			case ImportAssetUI::Texture:

				if (io::LoadAndImportTexture(selectedPath, selectedPath.parent_path().stem()))
				{
					std::cout << "Import Successful" << std::endl;
				}
				else
				{
					std::cout << "Import Failed" << std::endl;
				}

				mImportAssetUI = ImportAssetUI::Default;

				break;

			case ImportAssetUI::Default: break;
			}
		}

		// Update Selected Entity
		if (mWindowSceneHierarchy->GetEntityChanged())
		{
			mEntity = mWindowSceneHierarchy->GetSelectedEntity();

			for (const auto& window : mWindows)
			{
				window->SetSelectedEntity(mEntity);
			}
		}

		// Update Scene Data if any changes were made to an entity, and game is not currently playing
		if (mWindowNodeEditor->GetSceneChanged() && mEngine->GetPlayState() == core::PlayState::Stopped)
		{
			const auto sceneData = mEngine->GetSubsystem<io::SceneSerializationSubsystem>()->GetSceneData();

			sceneData->UpdateData(mEngine);
		}

		//ImGui_ImplVulkan_NewFrame();
		//ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		bool* pOpen = nullptr;

		// Show Dockspace with Menu Bar for Editor Windows
		ShowDockspace(pOpen);

		//ImGui::ShowDemoWindow(p_open);
		//ImPlot::ShowDemoWindow(p_open);

		// Draw UI Windows
		if (!mWindows.empty())
		{
			for (const auto& window : mWindows)
			{
				window->Draw(mEngine->GetDeltaTime());
			}
		}

		mFileDialog.Display();
	}

	bool EditorUISubsystem::ShouldUpdate()
	{
		return false;
	}

	void EditorUISubsystem::ShowDockspace(bool* open)
	{
		static bool optFullscreenPersistant = true;
		const bool optFullscreen = optFullscreenPersistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (optFullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
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

		if (optFullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		if (const ImGuiIO& io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			const ImGuiID dockspaceID = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		ShowMenuBar();

		if (mSaveScene)
		{
			mSaveScene = false;
			ImGui::OpenPopup("Save Scene");
		}

		const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5, 0.5));

		// Save Scene Modal Window
		if (ImGui::BeginPopupModal("Save Scene", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			const auto sceneData = mEngine->GetSubsystem<io::SceneSerializationSubsystem>()->GetSceneData();

			const std::string strName = sceneData->GetPath().string();
			std::vector name(256, '\0');
			for (int i = 0; i < strName.size(); i++)
			{
				name[i] = strName[i];
			}
			name.push_back('\0');

			ImGui::Text("Enter Scene Name:");
			if (ImGui::InputText("##Edit", &name[0], name.size(), ImGuiInputTextFlags_EnterReturnsTrue))
			{
				sceneData->SetPath(std::string(&name[0]));
			}

			if (ImGui::Button("Save"))
			{
				const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
				const auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();

				sceneData->UpdateData(mEngine);
				sceneData->Save();

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

	void EditorUISubsystem::ShowMenuBar()
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
					const auto settingsManager = mEngine->GetSubsystem<core::SettingsManager>();
                    settingsManager->Save(assets::AssetRegistry::Get()->GetProjectRoot() / "config" / "settings.toml");

					assets::AssetRegistry::Get()->SaveAssetCache();
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
					const auto sceneData = mEngine->GetSubsystem<io::SceneSerializationSubsystem>()->GetSceneData();

					const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
					const auto sceneGraph = mEngine->GetSubsystem<scene::SceneGraphSubsystem>();

					sceneData->UpdateData(mEngine);
					sceneData->Save();
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
					mEngine->Exit();
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
						ImGui::MenuItem(mWindows[i]->GetName().c_str(), NULL, mWindows[i]->GetShow());
					}
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
	}

	void EditorUISubsystem::SetStyle()
	{
		ImGuiStyle* style = &ImGui::GetStyle();
		ImVec4* colors = style->Colors;

		const auto hoverCol = ImVec4(0.6f, 0.0f, 1.0f, 1.000f);

		colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.01f, 0.01f, 0.01f, 1.000f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.000f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.01f, 0.01f, 0.01f, 1.000f);
		colors[ImGuiCol_Border] = ImVec4(0.0266f, 0.0266f, 0.0266f, 1.000f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.01f, 0.01f, 0.01f, 1.000f);
		colors[ImGuiCol_FrameBgHovered] = hoverCol;
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.0280f, 0.0280f, 0.0280f, 1.000f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.0148f, 0.0148f, 0.0148f, 1.000f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.0148f, 0.0148f, 0.0148f, 1.000f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0148f, 0.0148f, 0.0148f, 1.000f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.0195f, 0.0195f, 0.0195f, 1.000f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0160f, 0.0160f, 0.0160f, 1.000f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.0277f, 0.0277f, 0.0277f, 1.000f);
		colors[ImGuiCol_ScrollbarGrabHovered] = hoverCol;
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.0391f, 0.0391f, 0.0391f, 1.000f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
		colors[ImGuiCol_ButtonHovered] = hoverCol;
		colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
		colors[ImGuiCol_Header] = ImVec4(0.0313f, 0.0313f, 0.0313f, 1.000f);
		colors[ImGuiCol_HeaderHovered] = hoverCol;
		colors[ImGuiCol_HeaderActive] = ImVec4(0.0469f, 0.0469f, 0.0469f, 1.000f);
		colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
		colors[ImGuiCol_SeparatorHovered] = hoverCol;
		colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_Tab] = ImVec4(0.01f, 0.01f, 0.01f, 1.000f);
		colors[ImGuiCol_TabHovered] = hoverCol;
		colors[ImGuiCol_TabActive] = ImVec4(0.0195f, 0.0195f, 0.0195f, 1.000f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0195f, 0.0195f, 0.0195f, 1.000f);
		colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_PlotLinesHovered] = hoverCol;
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
		colors[ImGuiCol_PlotHistogramHovered] = hoverCol;
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

	void EditorUISubsystem::AddWindow(const std::shared_ptr<UIWindow>& window)
	{
		mWindows.push_back(window);
	}

	std::shared_ptr<UIWindowViewport> EditorUISubsystem::GetWindowViewport()
	{
		return mWindowViewport;
	}

	std::shared_ptr<UIWindowSettings> EditorUISubsystem::GetWindowSettings()
	{
		return mWindowSettings;
	}
}
