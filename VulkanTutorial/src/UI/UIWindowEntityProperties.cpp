#include <UI/UIWindowEntityProperties.h>

#include <Components/TransformComponent.h>
#include <Components/Rendering/MeshComponent.h>
#include <Components/Rendering/LightComponent.h>
#include <ECS/ECS.h>

#include <misc/cpp/imgui_stdlib.h>

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

					ImGuiTreeNodeFlags flags = 0;
					//flags |= ImGuiTreeNodeFlags_Framed;
					//flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
					flags |= ImGuiTreeNodeFlags_NoAutoOpenOnLog;
					flags |= ImGuiTreeNodeFlags_OpenOnArrow;
					flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
					flags |= ImGuiTreeNodeFlags_AllowItemOverlap;
					flags |= ImGuiTreeNodeFlags_DefaultOpen;
					flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

					sceneChanged = false;

					// Display Component UI
					DrawTransformUI(flags);
					DrawMeshUI(flags);
					DrawLightUI(flags);
					DrawRigidbodyUI(flags);
					DrawScriptUI(flags);

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
								comp.model_path = "content\\models\\cube.psm";
								comp.texture_path = "content\\textures\\cube.png";
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
								
								sceneChanged = true;
							}
						}

						if (ImGui::Selectable("Rigidbody Component"))
						{
							
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
				TransformComponent& transform = world->GetComponent<TransformComponent>(entity);

				if (ImGui::TreeNodeEx("Transform Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Transform"))
					{
						world->RemoveComponent<TransformComponent>(entity);
						sceneChanged = true;
					}

					if (ImGui::DragFloat3("Position", (float*)&transform.position, 0.1f))
					{
						positionChanged = true;
						sceneChanged = true;
					}

					if (ImGui::DragFloat3("Rotation", (float*)&transform.rotation, 0.1f))
					{
						sceneChanged = true;
					}

					if (ImGui::DragFloat3("Scale", (float*)&transform.scale, 0.1f))
					{
						sceneChanged = true;
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawMeshUI(ImGuiTreeNodeFlags flags)
		{
			// Display Mesh Component - If One Exists
			if (world->HasComponent<Rendering::MeshComponent>(entity))
			{
				if (ImGui::TreeNodeEx("Mesh Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					Rendering::MeshComponent& mesh = world->GetComponent<Rendering::MeshComponent>(entity);

					if (ImGui::SmallButton("X##Mesh"))
					{
						sceneChanged = true;
						world->SetComponentDeleted<Rendering::MeshComponent>(entity, true);
					}

					// Change Model Path
					ImGui::Text("Model Path:"); ImGui::SameLine();
					if (ImGui::Selectable((const char*)mesh.model_path.c_str(), false))
					{
						fileDialog->Open();
						modelSelected = true;
					}

					if (fileDialog->HasSelected() && modelSelected)
					{
						mesh.model_path = fileDialog->GetSelected().string();
						world->SetComponentInitialized<Rendering::MeshComponent>(entity, false);

						modelSelected = false;
						sceneChanged = true;
						fileDialog->ClearSelected();
					}

					// Change Texture Path
					ImGui::Text("Texture Path:"); ImGui::SameLine();
					if (ImGui::Selectable((const char*)mesh.texture_path.c_str(), false))
					{
						fileDialog->Open();
						textureSelected = true;
					}

					if (fileDialog->HasSelected() && textureSelected)
					{
						mesh.texture_path = fileDialog->GetSelected().string();
						world->SetComponentInitialized<Rendering::MeshComponent>(entity, false);

						textureSelected = false;
						sceneChanged = true;
						fileDialog->ClearSelected();
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawLightUI(ImGuiTreeNodeFlags flags)
		{
			if (world->HasComponent<Rendering::LightComponent>(entity))
			{
				Rendering::LightComponent& comp = world->GetComponent<Rendering::LightComponent>(entity);

				if (ImGui::TreeNodeEx("Light Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Light"))
					{
						sceneChanged = true;
						world->SetComponentDeleted<Rendering::LightComponent>(entity, true);
					}

					// Edit Light Diffuse Color
					ImGui::ColorEdit3("Diffuse", (float*)&comp.diffuseColor);

					// Edit Light Ambient Color
					ImGui::ColorEdit3("Ambient", (float*)&comp.ambientColor);

					if (ImGui::Checkbox("Cast Shadows", &comp.bFlagCastShadows))
					{
						sceneChanged = true;
						world->SetComponentInitialized<Rendering::LightComponent>(entity, false);
					}

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
						ImGui::DragFloat("Inner Cutoff Angle", &comp.innerCutoffAngle, 0.25f, 0.0f, 45.0f);

						// To avoid breaking the lighting, outerCutoffAngle should never be less than innerCutoffAngle
						ImGui::DragFloat("Outer Cutoff Angle", &comp.outerCutoffAngle, 0.25f, comp.innerCutoffAngle, 45.0f);

						// Outer Cutoff will match inner cutoff if inner cutoff vecomes larger
						if (comp.outerCutoffAngle < comp.innerCutoffAngle)
						{
							comp.outerCutoffAngle = comp.innerCutoffAngle;
						}
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawRigidbodyUI(ImGuiTreeNodeFlags flags)
		{
			/*if (world->HasComponent<Physics::RigidbodyComponent>(entity))
			{
				if (ImGui::TreeNodeEx("Rigidbody Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					Physics::RigidbodyComponent& rigidbody = world->GetComponent<Physics::RigidbodyComponent>(entity);

					if (ImGui::SmallButton("X##Rigidbody"))
					{
						rigidbody.bFlagDeleted = true;
						sceneChanged = true;
					}

					if (positionChanged)
					{
						rigidbody.bFlagCreated = true;
					}

					ImGui::TreePop();
				}
			}*/
		}

		void UIWindowEntityProperties::DrawScriptUI(ImGuiTreeNodeFlags flags)
		{
			if (world->HasComponent<Scripting::AngelScriptComponent>(entity))
			{
				Scripting::AngelScriptComponent& comp = world->GetComponent<Scripting::AngelScriptComponent>(entity);

				if (ImGui::TreeNodeEx("Script Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Script"))
					{
						world->SetComponentDeleted<Scripting::AngelScriptComponent>(entity, true);
						sceneChanged = true;
					}

					ImGui::InputText("Script Name", &comp.name);

					ImGui::Text("File Path:"); ImGui::SameLine(0.0f);
					if (ImGui::Selectable(comp.dir.string().c_str(), false))
					{
						//fileDialog->Open();
						//modelSelected = true;
					}

					if (comp.obj != 0)
					{
						ImGui::Separator();
						ImGui::Text("Editable Properties");

						// Display Variables in scripts marked as editable
						for (const int& index : comp.editableProperties)
						{
							// Get Property Name/Type
							const char* propertyName = comp.obj->GetPropertyName(index);
							int propertyType = comp.obj->GetPropertyTypeId(index);

							// Display property with correct UI based on its type
							if (propertyType == asTYPEID_INT32)
							{
								int* intProperty = (int*)comp.obj->GetAddressOfProperty(index);

								ImGui::InputInt(propertyName, intProperty);
							}
							else if (propertyType == asTYPEID_FLOAT)
							{
								float* floatProperty = (float*)comp.obj->GetAddressOfProperty(index);

								ImGui::InputFloat(propertyName, floatProperty);
							}
							else
							{
								std::string* stringProperty = (std::string*)comp.obj->GetAddressOfProperty(index);

								ImGui::InputText(propertyName, stringProperty);
							}
						}

						ImGui::Separator();
						ImGui::Text("Visible Properties");

						// Display Variables in scripts marked as visible
						for (const int& index : comp.visibleProperties)
						{
							// Get Property Name/Type
							const char* propertyName = comp.obj->GetPropertyName(index);
							int propertyType = comp.obj->GetPropertyTypeId(index);

							std::string propertyText = propertyName;

							// Display property with correct UI based on its type
							if (propertyType == asTYPEID_INT32)
							{
								int intProperty = *(int*)comp.obj->GetAddressOfProperty(index);
								propertyText += ": %d";

								ImGui::Text(propertyText.c_str(), intProperty);
							}
							else if (propertyType == asTYPEID_FLOAT)
							{
								float floatProperty = *(float*)comp.obj->GetAddressOfProperty(index);
								propertyText += ": %f";

								ImGui::Text(propertyText.c_str(), floatProperty);
							}
							else
							{
								std::string stringProperty = *(std::string*)comp.obj->GetAddressOfProperty(index);
								propertyText += ": %s";

								ImGui::Text(propertyText.c_str(), stringProperty.c_str());
							}
						}
					}

					ImGui::TreePop();
				}
			}
		}
	}
}