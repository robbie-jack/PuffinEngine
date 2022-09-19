#include "UIManager.h"

#include "Engine/Engine.hpp"

#include <ECS/ECS.h>
#include <ManipulationGizmo.h>

#include "SerializeScene.h"
#include "Assets/AssetRegistry.h"
#include "Assets/Importers/ModelImporter.h"
#include "Assets/Importers/TextureImporter.h"

#include <string>

namespace fs = std::filesystem;

namespace Puffin::UI
{
	UIManager::UIManager(std::shared_ptr<Core::Engine> engine)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		//ImPlot::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		SetStyle();

		saveScene = false;
		loadScene = false;
		importAssetUI = ImportAssetUI::None;

		m_engine = engine;

		windowSceneHierarchy =	std::make_shared<UIWindowSceneHierarchy>(m_engine);
		windowViewport = std::make_shared<UIWindowViewport>(m_engine);
		windowSettings = std::make_shared<UIWindowSettings>(m_engine);
		windowEntityProperties = std::make_shared<UIWindowEntityProperties>(m_engine);
		windowPerformance = std::make_shared<UIWindowPerformance>(m_engine);
		contentBrowser = std::make_shared<UIContentBrowser>(m_engine);

		windowEntityProperties->SetFileBrowser(&fileDialog);

		AddWindow(windowSceneHierarchy);
		AddWindow(windowSettings);
		AddWindow(windowEntityProperties);
		AddWindow(windowPerformance);
		AddWindow(contentBrowser);
	}

	void UIManager::Cleanup()
	{
		ImGui::DestroyContext();
		//ImPlot::DestroyContext();
	}

	void UIManager::DrawUI(double dt)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		bool* p_open = NULL;

		// Show Dockspace with Menu Bar for Editor Windows
		ShowDockspace(p_open);

		//ImGui::ShowDemoWindow(p_open);
		//ImPlot::ShowDemoWindow(p_open);

		// Draw UI Windows
		if (m_windows.size() > 0)
		{
			for (int i = 0; i < m_windows.size(); i++)
			{
				m_windows[i]->Draw(dt);
			}
		}

		fileDialog.Display();
	}

	// Any functionality which would cause errors/crashes from within 
	// VulkanRenderer DrawFrame() should be placed here instead
	void UIManager::Update()
	{
		if (fileDialog.HasSelected())
		{
			const fs::path selectedPath = fileDialog.GetSelected();

			// File Dialog - Load Scene
			if (loadScene)
			{
				m_engine->GetScene()->SetPath(selectedPath);

				m_engine->GetScene()->Load();

				m_engine->Restart();

				loadScene = false;
			}

			switch (importAssetUI)
			{
			// File Dialog - Import Mesh
			case ImportAssetUI::Mesh:

				if (IO::ImportMesh(selectedPath))
				{
					std::cout << "Import Successful" << std::endl;
				}
				else
				{
					std::cout << "Import Failed" << std::endl;
				}

				importAssetUI = ImportAssetUI::None;

				break;

			// File Dialog - Import Texture
			case ImportAssetUI::Texture:
					
				if (IO::ImportTexture(selectedPath))
				{
					std::cout << "Import Successful" << std::endl;
				}
				else
				{
					std::cout << "Import Failed" << std::endl;
				}

				importAssetUI = ImportAssetUI::None;

				break;
			}
		}

		// Update Selected Entity
		if (windowSceneHierarchy->HasEntityChanged())
		{
			m_entity = windowSceneHierarchy->GetEntity();
			windowEntityProperties->SetEntity(m_entity);
			windowViewport->SetEntity(m_entity);
		}

		// Update Scene Data if any changes were made to an entity, and game is not currently playing
		if (windowEntityProperties->HasSceneChanged() && m_engine->GetPlayState() == Core::PlayState::STOPPED)
		{
			m_engine->GetScene()->UpdateData();
		}
	}

	void UIManager::ShowDockspace(bool* p_open)
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
			ImGui::SetNextWindowPos(viewport->GetWorkPos());
			ImGui::SetNextWindowSize(viewport->GetWorkSize());
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
		ImGui::Begin("DockSpace Demo", p_open, window_flags);
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

		ShowMenuBar();

		if (saveScene)
		{
			saveScene = false;
			ImGui::OpenPopup("Save Scene");
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5, 0.5));

		// Save Scene Modal Window
		if (ImGui::BeginPopupModal("Save Scene", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			std::string str_name = m_engine->GetScene()->GetPath().string();
			std::vector<char> name(256, '\0');
			for (int i = 0; i < str_name.size(); i++)
			{
				name[i] = str_name[i];
			}
			name.push_back('\0');

			ImGui::Text("Enter Scene Name:");
			if (ImGui::InputText("##Edit", &name[0], name.size(), ImGuiInputTextFlags_EnterReturnsTrue))
			{
				m_engine->GetScene()->SetPath(std::string(&name[0]));
			}

			if (ImGui::Button("Save"))
			{
				m_engine->GetScene()->Save();
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

	void UIManager::ShowMenuBar()
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
					//IO::SaveProject(Assets::AssetRegistry::Get()->ProjectRoot() / Assets::AssetRegistry::Get()->ProjectName() + ".pproject", engine->)
					IO::SaveSettings(Assets::AssetRegistry::Get()->ProjectRoot() / "settings.json", m_engine->GetProjectSettings());
					Assets::AssetRegistry::Get()->SaveAssetCache();
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
					fileDialog.Open();
					loadScene = true;
				}

				if (ImGui::MenuItem("Save Scene"))
				{
					m_engine->GetScene()->Save();
				}

				if (ImGui::MenuItem("Save Scene As"))
				{
					saveScene = true;
				}

				ImGui::Text("---Other---");

				if (ImGui::BeginMenu("Import"))
				{
					if (ImGui::MenuItem("Mesh"))
					{
						fileDialog.Open();
						importAssetUI = ImportAssetUI::Mesh;
					}

					if (ImGui::MenuItem("Texture"))
					{
						fileDialog.Open();
						importAssetUI = ImportAssetUI::Texture;
					}

					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Quit", "Alt+F4"))
				{
					m_engine->Exit();
				}

				ImGui::EndMenu();
			}

			// List all windows
			if (ImGui::BeginMenu("Windows"))
			{
				if (m_windows.size() > 0)
				{
					for (int i = 0; i < m_windows.size(); i++)
					{
						// Show/Hide window if clicked
						ImGui::MenuItem(m_windows[i]->GetName().c_str(), NULL, m_windows[i]->GetShow());
					}
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
	}

	void UIManager::SetStyle()
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

	void UIManager::AddWindow(std::shared_ptr<UIWindow> window)
	{
		m_windows.push_back(window);
	}
}