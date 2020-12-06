#include "UIManager.h"
#include "SerializeScene.h"
#include "ModelLoader.h"

#include <string>

namespace Puffin
{
	namespace UI
	{
		UIManager::UIManager()
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			//ImPlot::CreateContext();

			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

			SetStyle();

			running = true;
			saveScene = false;
			loadScene = false;
			importMesh = false;
			playButtonLabel = "Play";

			windowSceneHierarchy = new UIWindowSceneHierarchy();
			windowViewport = new UIWindowViewport();
			windowSettings = new UIWindowSettings();
			windowEntityProperties = new UIWindowEntityProperties();
			windowPerformance = new UIWindowPerformance();

			//windowPerformance->Show();

			windowSceneHierarchy->SetWindowProperties(windowEntityProperties);
			windowEntityProperties->SetFileBrowser(&fileDialog);

			windows.push_back(windowSceneHierarchy);
			windows.push_back(windowViewport);
			windows.push_back(windowSettings);
			windows.push_back(windowEntityProperties);
			windows.push_back(windowPerformance);
		}

		UIManager::~UIManager()
		{
			
		}

		void UIManager::Cleanup()
		{
			ImGui::DestroyContext();
			//ImPlot::DestroyContext();
		}

		bool UIManager::DrawUI(float dt, Input::InputManager* InputManager)
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			bool* p_open = NULL;

			// Show Dockspace with Menu Bar for Editor Windows
			if (!ShowDockspace(p_open))
			{
				running = false;
			}

			//ImGui::ShowDemoWindow(p_open);
			//ImPlot::ShowDemoWindow(p_open);

			// Draw UI Windows
			if (windows.size() > 0)
			{
				for (int i = 0; i < windows.size(); i++)
				{
					if (!windows[i]->Draw(dt, InputManager))
					{
						running = false;
					}
				}
			}

			fileDialog.Display();

			return running;
		}

		// Any functionality which would cause errors/crashes from within 
		// VulkanRenderer DrawFrame() should be placed here instead
		void UIManager::Update()
		{
			// File Dialog - Load Scene
			if (fileDialog.HasSelected() && loadScene)
			{
				std::string scenePath = fileDialog.GetSelected().string();
				engine->GetScene().scene_name = scenePath;

				IO::LoadScene(world, engine->GetScene());

				engine->Restart();

				loadScene = false;
			}

			// File Dialog - Import Mesh
			if (fileDialog.HasSelected() && importMesh)
			{
				std::string importPath = fileDialog.GetSelected().string();
				if (IO::ImportMesh(importPath))
				{
					std::cout << "Import Successful" << std::endl;
				}
				else
				{
					std::cout << "Import Failed" << std::endl;
				}

				importMesh = false;
			}

			// Update Scene Data if any changes were made to an entity
			if (windowEntityProperties->HasSceneChanged())
			{
				IO::UpdateSceneData(world, engine->GetScene());
			}
		}

		bool UIManager::ShowDockspace(bool* p_open)
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
			else
			{
				//ShowDockingDisabledMessage();
			}

			bool running = true;

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
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
						IO::SaveScene(world, engine->GetScene());
					}

					if (ImGui::MenuItem("Save Scene As"))
					{
						saveScene = true;
					}

					if (ImGui::BeginMenu("Import"))
					{
						if (ImGui::MenuItem("Mesh"))
						{
							fileDialog.Open();
							importMesh = true;
						}

						ImGui::EndMenu();
					}

					if (ImGui::MenuItem("Quit", "Alt+F4"))
					{
						running = false;
					}

					ImGui::EndMenu();
				}

				// List all windows
				if (ImGui::BeginMenu("Windows"))
				{
					if (windows.size() > 0)
					{
						for (int i = 0; i < windows.size(); i++)
						{
							// Show/Hide window if clicked
							ImGui::MenuItem(windows[i]->GetName().c_str(), NULL, windows[i]->GetShow());
						}
					}

					ImGui::EndMenu();
				}

				ImGui::Dummy(ImVec2((ImGui::GetWindowWidth() / 2) - 200.0f, 0.0f));

				if (ImGui::Button(playButtonLabel.c_str()))
				{
					engine->Play();

					if (engine->GetPlayState() == PlayState::PAUSED)
					{
						playButtonLabel = "Play";
					}
					else if (engine->GetPlayState() == PlayState::PLAYING)
					{
						playButtonLabel = "Pause";
					}
				}

				if (ImGui::Button("Stop"))
				{
					engine->Restart();
					playButtonLabel = "Play";
				}

				ImGui::EndMenuBar();
			}

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
				std::string str_name = engine->GetScene().scene_name;
				std::vector<char> name(256, '\0');
				for (int i = 0; i < str_name.size(); i++)
				{
					name[i] = str_name[i];
				}
				name.push_back('\0');

				ImGui::Text("Enter Scene Name:");
				if (ImGui::InputText("##Edit", &name[0], name.size(), ImGuiInputTextFlags_EnterReturnsTrue))
				{
					engine->GetScene().scene_name = std::string(&name[0]);
				}

				if (ImGui::Button("Save"))
				{
					IO::SaveScene(world, engine->GetScene());
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

			return running;
		}

		void UIManager::SetStyle()
		{
			ImGuiStyle* style = &ImGui::GetStyle();
			ImVec4* colors = style->Colors;

			colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
			colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
			colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
			colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
			colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
			colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
			colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
			colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
			colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
			colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
			colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
			colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
			colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
			colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
			colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
			colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
			colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
			colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
			colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
			colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
			colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
			colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
			colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
			colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
			colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
			colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
			colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
			colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
			colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
			colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
			colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
			colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
			colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
			colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
			colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
			colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
			colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
			colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
			colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
			colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
			colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
			colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
			colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
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

		void UIManager::AddWindow(UIWindow* window)
		{
			windows.push_back(window);
		}
	}
}