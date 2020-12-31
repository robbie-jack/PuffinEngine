#include "UIWindowEntityProperties.h"

#include "TransformComponent.h"
#include "MeshComponent.h"
#include "LightComponent.h"
#include "RigidbodyComponent.h"

#include "imgui/imgui_stdlib.h"

#include <iostream>
#include <string>

namespace Puffin
{
	namespace UI
	{
		bool UIWindowEntityProperties::Draw(float dt, Puffin::Input::InputManager* InputManager)
		{
			windowName = "Entity Properties";

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				Begin(windowName);

				if (entity != ECS::INVALID_ENTITY)
				{
					ImGui::Dummy(ImVec2(0.0f, 5.0f));
					ImGui::Text(""); ImGui::SameLine(0.0f);

					// Edit Entity Name
					std::string name = world->GetEntityName(entity);
					std::string* namePtr = &name;
					if (ImGui::InputText("##Name", namePtr, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						world->SetEntityName(entity, *namePtr);
					}

					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::Text(" List of Components Here");

					// List of all Entity Components
					ImVec2 listBoxSize = ImGui::GetWindowSize();
					listBoxSize.y -= 110.0f;

					ImGui::ListBoxHeader("", listBoxSize); // Make ListBox fill Window

					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
						| ImGuiTreeNodeFlags_OpenOnDoubleClick
						| ImGuiTreeNodeFlags_AllowItemOverlap;

					sceneChanged = false;

					// Display Component UI
					DrawTransformUI(flags);
					DrawMeshUI(flags);
					DrawLightUI(flags);
					DrawRigidbodyUI(flags);

					positionChanged = false;

					ImGui::ListBoxFooter();

					ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2 - 50.0f, 0.0f)); ImGui::SameLine();

					if (ImGui::Button("Add Component"))
					{
						ImGui::OpenPopup("Add Component");
					}

					// Display Add Component Popup
					if (ImGui::BeginPopup("Add Component"))
					{
						if (ImGui::Selectable("Transform Component"))
						{
							if (!world->HasComponent<TransformComponent>(entity))
							{
								world->AddComponent<TransformComponent>(entity);
								sceneChanged = true;
							}
						}

						if (ImGui::Selectable("Mesh Component"))
						{
							if (!world->HasComponent<Rendering::MeshComponent>(entity))
							{
								Rendering::MeshComponent& comp = world->AddComponent<Rendering::MeshComponent>(entity);
								comp.model_path = "assets\\models\\cube.asset_m";
								comp.texture_path = "textures\\cube.png";
								comp.flag_created = true;
								sceneChanged = true;
							}
						}

						if (ImGui::Selectable("Light Component"))
						{
							if (!world->HasComponent<Rendering::LightComponent>(entity))
							{
								Rendering::LightComponent& comp = world->AddComponent<Rendering::LightComponent>(entity);
								comp.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
								comp.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
								comp.innerCutoffAngle = 12.5f;
								comp.outerCutoffAngle = 17.5f;
								comp.constantAttenuation = 1.0f;
								comp.linearAttenuation = 0.09f;
								comp.quadraticAttenuation = 0.032f;
								comp.specularStrength = 0.5f;
								comp.shininess = 16;
								comp.type = Rendering::LightType::POINT;
								comp.flag_created = true;
								sceneChanged = true;
							}
						}

						if (ImGui::Selectable("Physics Component"))
						{
							if (!world->HasComponent<Physics::RigidbodyComponent>(entity))
							{
								Physics::RigidbodyComponent& comp = world->AddComponent<Physics::RigidbodyComponent>(entity);
								comp.size = btVector3(1.0f, 1.0f, 1.0f);
								comp.mass = 0.0f;
								comp.flag_created = true;
								sceneChanged = true;
							}
						}

						ImGui::EndPopup();
					}
				}
				else
				{
					ImGui::Text(" No Entity Selected");
				}

				End();
			}

			return true;
		}

		void UIWindowEntityProperties::DrawTransformUI(ImGuiTreeNodeFlags flags)
		{
			// Display Transform Component - If One Exists
			if (world->HasComponent<TransformComponent>(entity))
			{
				float PI = 3.14159;

				TransformComponent& transform = world->GetComponent<TransformComponent>(entity);
				float position[3] = { transform.position.x, transform.position.y, transform.position.z };
				float rotation[3] = {
					transform.rotation.x * 180 / PI,
					transform.rotation.y * 180 / PI,
					transform.rotation.z * 180 / PI };
				float scale[3] = { transform.scale.x, transform.scale.y, transform.scale.z };

				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::CollapsingHeader("Transform Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					ImGui::Button("X");

					if (ImGui::IsItemActivated())
					{
						world->RemoveComponent<TransformComponent>(entity);
						sceneChanged = true;
					}

					if (ImGui::DragFloat3("Position", position, 0.1f))
					{
						transform.position.x = position[0];
						transform.position.y = position[1];
						transform.position.z = position[2];

						positionChanged = true;
						sceneChanged = true;
					}

					if (ImGui::DragFloat3("Rotation", rotation, 0.1f))
					{
						transform.rotation.x = rotation[0] * PI / 180;
						transform.rotation.y = rotation[1] * PI / 180;
						transform.rotation.z = rotation[2] * PI / 180;

						sceneChanged = true;
					}

					if (ImGui::DragFloat3("Scale", scale, 0.1f))
					{
						transform.scale.x = scale[0];
						transform.scale.y = scale[1];
						transform.scale.z = scale[2];

						sceneChanged = true;
					}
				}
			}
		}

		void UIWindowEntityProperties::DrawMeshUI(ImGuiTreeNodeFlags flags)
		{
			// Display Mesh Component - If One Exists
			if (world->HasComponent<Rendering::MeshComponent>(entity))
			{
				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::CollapsingHeader("Mesh Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					Rendering::MeshComponent& mesh = world->GetComponent<Rendering::MeshComponent>(entity);

					ImGui::Button("X");

					if (ImGui::IsItemClicked())
					{
						mesh.flag_deleted = true;
						sceneChanged = true;
					}

					// Change Model Path
					ImGui::Text("Model Path:"); ImGui::SameLine(100.0f);
					if (ImGui::Selectable(mesh.model_path.c_str(), false))
					{
						fileDialog->Open();
						modelSelected = true;
					}

					if (fileDialog->HasSelected() && modelSelected)
					{
						mesh.model_path = fileDialog->GetSelected().string();
						mesh.flag_created = true;
						modelSelected = false;
						sceneChanged = true;
						fileDialog->ClearSelected();
					}

					// Change Texture Path
					ImGui::Text("Texture Path:"); ImGui::SameLine(100.0f);
					if (ImGui::Selectable(mesh.texture_path.c_str(), false))
					{
						fileDialog->Open();
						textureSelected = true;
					}

					if (fileDialog->HasSelected() && textureSelected)
					{
						mesh.texture_path = fileDialog->GetSelected().string();
						mesh.flag_created = true;
						textureSelected = false;
						sceneChanged = true;
						fileDialog->ClearSelected();
					}
				}
			}
		}

		void UIWindowEntityProperties::DrawLightUI(ImGuiTreeNodeFlags flags)
		{
			if (world->HasComponent<Rendering::LightComponent>(entity))
			{
				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::CollapsingHeader("Light Component"), flags)
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					Rendering::LightComponent& comp = world->GetComponent<Rendering::LightComponent>(entity);

					ImGui::Button("X");

					if (ImGui::IsItemClicked())
					{
						comp.flag_deleted = true;
						sceneChanged = true;
					}

					// Edit Light Diffuse Color
					ImGui::ColorEdit3("Diffuse", (float*)&comp.diffuseColor);

					// Edit Light Ambient Color
					ImGui::ColorEdit3("Ambient", (float*)&comp.ambientColor);

					ImGui::Checkbox("Cast Shadows", &comp.castShadows);

					// Combo box to select light type
					const char* items[] = { "Point", "Spot", "Directional" };
					int item_current_idx = (int)comp.type;
					const char* label = items[item_current_idx];
					if (ImGui::BeginCombo("Light Type", label))
					{
						for (int i = 0; i < IM_ARRAYSIZE(items); i++)
						{
							const bool is_selected = (item_current_idx == i);
							if (ImGui::Selectable(items[i], is_selected))
							{
								item_current_idx = i;
								comp.type = (Rendering::LightType)item_current_idx;
								sceneChanged = true;
							}

							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}

						ImGui::EndCombo();
					}

					// Render Direction Edit UI if  light type is Direction or Spot
					if (comp.type != Rendering::LightType::POINT)
					{
						ImGui::DragFloat3("Direction", (float*)&comp.direction, 0.005f, -1.0f, 1.0f);
					}

					if (comp.type == Rendering::LightType::SPOT)
					{
						ImGui::DragFloat("Inner Cutoff Angle", &comp.innerCutoffAngle, 0.25f, 0.0f, 180.0f);

						// To avoid breaking the lighting, outerCutoffAngle should never be less than innerCutoffAngle
						ImGui::DragFloat("Outer Cutoff Angle", &comp.outerCutoffAngle, 0.25f, comp.innerCutoffAngle, 180.0f);
					}
				}
			}
		}

		void UIWindowEntityProperties::DrawRigidbodyUI(ImGuiTreeNodeFlags flags)
		{
			if (world->HasComponent<Physics::RigidbodyComponent>(entity))
			{
				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
				if (ImGui::CollapsingHeader("Physics Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					Physics::RigidbodyComponent& comp = world->GetComponent<Physics::RigidbodyComponent>(entity);

					ImGui::Button("X");

					if (ImGui::IsItemClicked())
					{
						comp.flag_deleted = true;
						sceneChanged = true;
					}

					if (positionChanged)
					{
						comp.flag_created = true;
					}
				}
			}
		}
	}
}