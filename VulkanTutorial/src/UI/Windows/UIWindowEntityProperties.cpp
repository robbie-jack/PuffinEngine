#include "UIWindowEntityProperties.h"
#include "Engine/Engine.hpp"

#include "Components/TransformComponent.h"
#include "Components/Rendering/MeshComponent.h"
#include "Components/Rendering/LightComponent.h"
#include "Components/Physics/Box2D/Box2DRigidbodyComponent.h"
#include "Components/Physics/Box2D/Box2DShapeComponents.h"
#include "Components/AngelScriptComponent.h"

#include "ECS/ECS.h"

#include "Types/ComponentFlags.h"

#include <misc/cpp/imgui_stdlib.h>

#include <iostream>
#include <string>

namespace Puffin
{
	namespace UI
	{
		void UIWindowEntityProperties::Draw(double dt)
		{
			windowName = "Entity Properties";

			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();

			if (show)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				Begin(windowName);

				if (m_entity != ECS::INVALID_ENTITY)
				{
					ImGui::Dummy(ImVec2(0.0f, 5.0f));
					ImGui::Text(""); ImGui::SameLine(0.0f);

					// Edit Entity Name
					std::string name = ecsWorld->GetEntityName(m_entity);
					std::string* namePtr = &name;
					if (ImGui::InputText("##Name", namePtr, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						ecsWorld->SetEntityName(m_entity, *namePtr);
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
					
					DrawPointLightUI(flags);
					DrawDirectionalLightUI(flags);
					DrawSpotLightUI(flags);

					DrawRigidbody2DUI(flags);
					DrawCircle2DUI(flags);
					DrawBox2DUI(flags);

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
						if (ImGui::Selectable("Transform"))
						{
							if (!ecsWorld->HasComponent<TransformComponent>(m_entity))
							{
								ecsWorld->AddComponent<TransformComponent>(m_entity);
								sceneChanged = true;
							}
						}

						if (ImGui::Selectable("Static Mesh"))
						{
							if (!ecsWorld->HasComponent<Rendering::MeshComponent>(m_entity))
							{
								ecsWorld->AddComponent<Rendering::MeshComponent>(m_entity);
								sceneChanged = true;
							}
						}

						/*if (ImGui::Selectable("Light"))
						{
							if (!ecsWorld->HasComponent<Rendering::LightComponent>(m_entity))
							{
								auto& light = ecsWorld->AddAndGetComponent<Rendering::LightComponent>(m_entity);
								light.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
								light.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
								light.innerCutoffAngle = 12.5f;
								light.outerCutoffAngle = 17.5f;
								light.constantAttenuation = 1.0f;
								light.linearAttenuation = 0.09f;
								light.quadraticAttenuation = 0.032f;
								light.specularStrength = 0.5f;
								light.shininess = 16;
								light.type = Rendering::LightType::POINT;
								
								sceneChanged = true;
							}
						}*/
						if (ImGui::BeginPopup("Lights"))
						{
							if (ImGui::Selectable("Point Light"))
							{
								ecsWorld->AddComponent<Rendering::PointLightComponent>(m_entity);
							}

							if (ImGui::Selectable("Directional Light"))
							{
								ecsWorld->AddComponent<Rendering::DirectionalLightComponent>(m_entity);
							}

							if (ImGui::Selectable("Spot Light"))
							{
								ecsWorld->AddComponent<Rendering::SpotLightComponent>(m_entity);
							}
						}

						if (ImGui::BeginPopup("Physics"))
						{
							if (ImGui::Selectable("Rigidbody"))
							{
								ecsWorld->AddComponent<Physics::Box2DRigidbodyComponent>(m_entity);
							}

							if (ImGui::Selectable("Circle2D"))
							{
								ecsWorld->AddComponent<Physics::Box2DCircleComponent>(m_entity);
							}

							if (ImGui::Selectable("Box2D"))
							{
								ecsWorld->AddComponent<Physics::Box2DBoxComponent>(m_entity);
							}

							ImGui::EndPopup();
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
		}

		void UIWindowEntityProperties::DrawTransformUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();

			// Display Transform Component - If One Exists
			if (ecsWorld->HasComponent<TransformComponent>(m_entity))
			{
				TransformComponent& transform = ecsWorld->GetComponent<TransformComponent>(m_entity);

				if (ImGui::TreeNodeEx("Transform Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Transform"))
					{
						ecsWorld->RemoveComponent<TransformComponent>(m_entity);
						sceneChanged = true;
					}

					#ifdef PFN_USE_DOUBLE_PRECISION
						if (ImGui::DragScalarN("Position", ImGuiDataType_Double, (float*)&transform.position, 0.1f))
						{
							positionChanged = true;
							sceneChanged = true;
						}
					#else
						if (ImGui::DragFloat3("Position", (float*)&transform.position, 0.1f))
						{
							positionChanged = true;
							sceneChanged = true;
						}
					#endif

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
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();

			// Display Mesh Component - If One Exists
			if (ecsWorld->HasComponent<Rendering::MeshComponent>(m_entity))
			{
				if (ImGui::TreeNodeEx("Mesh Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					Rendering::MeshComponent& mesh = ecsWorld->GetComponent<Rendering::MeshComponent>(m_entity);

					if (ImGui::SmallButton("X##Mesh"))
					{
						sceneChanged = true;
						ecsWorld->SetComponentFlag<Rendering::MeshComponent, FlagDeleted>(m_entity, true);
					}

					ImGui::Text("Model UUID: %d", mesh.meshAssetID);
					ImGui::Text("Texture UUID: %d", mesh.textureAssetID);

					// Change Model Path
					/*ImGui::Text("Model Path:"); ImGui::SameLine();
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
					}*/

					// Change Texture Path
					/*ImGui::Text("Texture Path:"); ImGui::SameLine();
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
					}*/

					ImGui::TreePop();
				}
			}
		}

		bool UIWindowEntityProperties::DrawLightUI(Rendering::LightComponent* light)
		{
			bool dirty = false;

			if (light != nullptr)
			{
				// Edit Light Diffuse Color
				if (ImGui::ColorEdit3("Diffuse", (float*)&light->diffuseColor))
				{
					dirty = true;
				}

				// Edit Light Ambient Color
				if (ImGui::ColorEdit3("Ambient", (float*)&light->ambientColor))
				{
					dirty = true;
				}
			}

			return dirty;
		}

		void UIWindowEntityProperties::DrawPointLightUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Rendering::PointLightComponent>(m_entity))
			{
				auto& light = ecsWorld->GetComponent<Rendering::PointLightComponent>(m_entity);
				bool dirty = false;

				if (ImGui::TreeNodeEx("Point Light Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Point Light"))
					{
						ecsWorld->SetComponentFlag<Rendering::PointLightComponent, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					if (DrawLightUI(&light))
					{
						dirty = true;
					}

					if (ImGui::DragFloat("Linear Attenuation", &light.linearAttenuation, .01f, .01f, 1.f, "%.4f"))
					{
						dirty = true;
					}

					if (ImGui::DragFloat("Quadratic Attenuation", &light.quadraticAttenuation, .01f, .01f, 2.f, "%.6f"))
					{
						dirty = true;
					}

					ImGui::TreePop();
				}

				if (dirty)
				{
					sceneChanged = true;
					ecsWorld->SetComponentFlag<Rendering::PointLightComponent, FlagDeleted>(m_entity, true);
				}
			}
		}

		void UIWindowEntityProperties::DrawDirectionalLightUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Rendering::DirectionalLightComponent>(m_entity))
			{
				auto& light = ecsWorld->GetComponent<Rendering::DirectionalLightComponent>(m_entity);
				bool dirty = false;

				if (ImGui::TreeNodeEx("Directional Light Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Directional Light"))
					{
						ecsWorld->SetComponentFlag<Rendering::PointLightComponent, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					if (DrawLightUI(&light))
					{
						dirty = true;
					}

					if (ImGui::DragFloat3("Direction", (float*)&light.direction, 0.005f, -1.0f, 1.0f))
					{
						dirty = true;
					}

					ImGui::TreePop();
				}

				if (dirty)
				{
					sceneChanged = true;
					ecsWorld->SetComponentFlag<Rendering::PointLightComponent, FlagDeleted>(m_entity, true);
				}
			}
		}

		void UIWindowEntityProperties::DrawSpotLightUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Rendering::SpotLightComponent>(m_entity))
			{
				auto& light = ecsWorld->GetComponent<Rendering::SpotLightComponent>(m_entity);
				bool dirty = false;

				if (ImGui::TreeNodeEx("Spot Light Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Spot Light"))
					{
						ecsWorld->SetComponentFlag<Rendering::SpotLightComponent, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					if (DrawLightUI(&light))
					{
						sceneChanged = true;
						ecsWorld->SetComponentFlag<Rendering::SpotLightComponent, FlagDeleted>(m_entity, true);
					}

					if (ImGui::DragFloat3("Direction", (float*)&light.direction, 0.005f, -1.0f, 1.0f))
					{
						dirty = true;
					}

					if (ImGui::DragFloat("Linear Attenuation", &light.linearAttenuation, .01f, .01f, 1.f, "%.4f"))
					{
						dirty = true;
					}

					if (ImGui::DragFloat("Quadratic Attenuation", &light.quadraticAttenuation, .01f, .01f, 2.f, "%.6f"))
					{
						dirty = true;
					}

					if (ImGui::DragFloat("Inner Cutoff Angle", &light.innerCutoffAngle, 0.25f, 0.0f, 45.0f))
					{
						dirty = true;
					}

					// To avoid breaking the lighting, outerCutoffAngle should never be less than innerCutoffAngle
					if (ImGui::DragFloat("Outer Cutoff Angle", &light.outerCutoffAngle, 0.25f, light.innerCutoffAngle, 45.0f))
					{
						dirty = true;
					}

					// Outer Cutoff will match inner cutoff if inner cutoff becomes larger
					if (light.outerCutoffAngle < light.innerCutoffAngle)
					{
						light.outerCutoffAngle = light.innerCutoffAngle;
					}

					ImGui::TreePop();
				}

				if (dirty)
				{
					sceneChanged = true;
					ecsWorld->SetComponentFlag<Rendering::PointLightComponent, FlagDeleted>(m_entity, true);
				}
			}
		}

		void UIWindowEntityProperties::DrawRigidbody2DUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Physics::Box2DRigidbodyComponent>(m_entity))
			{
				if (ImGui::TreeNodeEx("Rigidbody Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					auto& rb = ecsWorld->GetComponent<Physics::Box2DRigidbodyComponent>(m_entity);

					if (ImGui::SmallButton("X##Rigidbody"))
					{
						ecsWorld->SetComponentFlag<Physics::Box2DRigidbodyComponent, FlagDeleted>(m_entity, true);
							
						sceneChanged = true;
					}

					// Edit Body Properties
					if (rb.body != nullptr)
					{
						auto* body = rb.body;
						bool bodyChanged = false;

						// Combo box for Body Type Selection
						const char* items[] = { "Static", "Kinematic", "Dynamic" };
						int item_current_idx = (int)body->GetType();
						const char* label = items[item_current_idx];
						if (ImGui::BeginCombo("Body Type", label))
						{
							for (int i = 0; i < IM_ARRAYSIZE(items); i++)
							{
								const bool isSelected = (item_current_idx == i);
								if (ImGui::Selectable(items[i], isSelected))
								{
									item_current_idx = i;
									body->SetType(static_cast<b2BodyType>(item_current_idx));
									bodyChanged = true;
								}

								if (isSelected)
								{
									ImGui::SetItemDefaultFocus();
								}
							}

							ImGui::EndCombo();
						}

						// Sleep
						bool sleep = body->IsSleepingAllowed();
						if (ImGui::Checkbox("Sleeping Allowed", &sleep))
						{
							body->SetSleepingAllowed(sleep);
							bodyChanged = true;
						}

						// Bullet
						bool bullet = body->IsBullet();
						if (ImGui::Checkbox("Bullet", &bullet))
						{
							body->SetBullet(bullet);
							bodyChanged = true;
						}

						// Awake
						bool awake = body->IsAwake();
						if (ImGui::Checkbox("Awake", &awake))
						{
							body->SetAwake(awake);
							bodyChanged = true;
						}

						// Enabled
						bool enabled = body->IsEnabled();
						if (ImGui::Checkbox("Enabled", &enabled))
						{
							body->SetEnabled(enabled);
							bodyChanged = true;
						}

						// Fixed Rotation
						bool fixedRotation = body->IsFixedRotation();
						if (ImGui::Checkbox("Rotation Fixed", &fixedRotation))
						{
							body->SetFixedRotation(fixedRotation);
							bodyChanged = true;
						}

						// Angular Damping
						float angularDamping = body->GetAngularDamping();
						if (ImGui::DragFloat("Angular Damping", &angularDamping))
						{
							body->SetAngularDamping(angularDamping);
							bodyChanged = true;
						}

						// Linear Damping
						float linearDamping = body->GetLinearDamping();
						if (ImGui::DragFloat("Linear Damping", &linearDamping))
						{
							body->SetLinearDamping(linearDamping);
							bodyChanged = true;
						}

						// Gravity Scale
						float gravityScale = body->GetGravityScale();
						if (ImGui::DragFloat("Gravity Scale", &gravityScale))
						{
							body->SetGravityScale(gravityScale);
							bodyChanged = true;
						}

						if(bodyChanged)
						{
							Physics::UpdateBodyDef(rb);
							sceneChanged = true;
						}
					}

					// Edit Fixture Properties
					if (rb.fixture != nullptr)
					{
						auto* fixture = rb.fixture;
						bool fixtureChanged = false;

						// Density
						float density = fixture->GetDensity();
						if (ImGui::DragFloat("Density", &density))
						{
							fixture->SetDensity(density);
							fixtureChanged = true;
						}

						// Friction
						float friction = fixture->GetFriction();
						if (ImGui::DragFloat("Friction", &friction))
						{
							fixture->SetFriction(friction);
							fixtureChanged = true;
						}

						// Restitution/Elasticity
						float restitution = fixture->GetRestitution();
						if (ImGui::DragFloat("Restitution/Elasticity", &restitution))
						{
							fixture->SetRestitution(restitution);
							fixtureChanged = true;
						}

						// Restitution/Elasticity Threshold
						float restitutionThreshold = fixture->GetRestitutionThreshold();
						if (ImGui::DragFloat("Restitution/Elasticity Threshold", &restitutionThreshold))
						{
							fixture->SetRestitutionThreshold(restitutionThreshold);
							fixtureChanged = true;
						}

						if (fixtureChanged)
						{
							Physics::UpdateFixtureDef(rb);
							sceneChanged = true;
						}
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawCircle2DUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Physics::Box2DCircleComponent>(m_entity))
			{
				if (ImGui::TreeNodeEx("Circle Component 2D", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					auto& circle = ecsWorld->GetComponent<Physics::Box2DCircleComponent>(m_entity);

					if (ImGui::SmallButton("X##Circle2D"))
					{
						ecsWorld->SetComponentFlag<Physics::Box2DCircleComponent, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					if (circle.shape != nullptr)
					{
						auto* shape = circle.shape;

						if (ImGui::DragFloat("Radius", &shape->m_radius))
						{
							circle.data.radius = shape->m_radius;
							sceneChanged = true;
						}
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawBox2DUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Physics::Box2DBoxComponent>(m_entity))
			{
				if (ImGui::TreeNodeEx("Box Component 2D", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					auto& box = ecsWorld->GetComponent<Physics::Box2DBoxComponent>(m_entity);

					if (ImGui::SmallButton("X##Box2D"))
					{
						ecsWorld->SetComponentFlag< Physics::Box2DBoxComponent, FlagDeleted>(m_entity, true);

						sceneChanged = true;
					}

					if (box.shape != nullptr)
					{
						auto* shape = box.shape;

						if (ImGui::DragFloat2("Half Extent", reinterpret_cast<float*>(&box.data.halfExtent), 0.1f, 0.0f))
						{
							shape->SetAsBox(box.data.halfExtent.x, box.data.halfExtent.y);
							sceneChanged = true;
						}
					}

					ImGui::TreePop();
				}
			}
		}

		void UIWindowEntityProperties::DrawScriptUI(ImGuiTreeNodeFlags flags)
		{
			auto ecsWorld = m_engine->GetSubsystem<ECS::World>();
			if (ecsWorld->HasComponent<Scripting::AngelScriptComponent>(m_entity))
			{
				Scripting::AngelScriptComponent& comp = ecsWorld->GetComponent<Scripting::AngelScriptComponent>(m_entity);

				if (ImGui::TreeNodeEx("Script Component", flags))
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

					if (ImGui::SmallButton("X##Script"))
					{
						ecsWorld->SetComponentFlag<Scripting::AngelScriptComponent, FlagDeleted>(m_entity, true);
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
							if (propertyType == asTYPEID_BOOL)
							{
								bool* boolProperty = (bool*)comp.obj->GetAddressOfProperty(index);

								ImGui::Checkbox(propertyName, boolProperty);
							}
							else if (propertyType == asTYPEID_INT32)
							{
								int* intProperty = (int*)comp.obj->GetAddressOfProperty(index);

								ImGui::InputInt(propertyName, intProperty);
							}
							else if (propertyType == asTYPEID_FLOAT)
							{
								float* floatProperty = (float*)comp.obj->GetAddressOfProperty(index);

								ImGui::InputFloat(propertyName, floatProperty);
							}
							else if (propertyType == asTYPEID_DOUBLE)
							{
								double* doubleProperty = (double*)comp.obj->GetAddressOfProperty(index);

								ImGui::InputDouble(propertyName, doubleProperty);
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
								propertyText += ": %.20f";

								ImGui::Text(propertyText.c_str(), floatProperty);
							}
							else if (propertyType == asTYPEID_DOUBLE)
							{
								double doubleProperty = *(double*)comp.obj->GetAddressOfProperty(index);
								propertyText += ": %.20f";

								ImGui::Text(propertyText.c_str(), doubleProperty);
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