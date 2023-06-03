#include "UI/Editor/Windows/UIWindowEntityProperties.h"

#include "Components/TransformComponent.h"
#include "Components/Physics/RigidbodyComponent2D.h"
#include "Components/Physics/ShapeComponents2D.h"
#include "Components/Procedural/ProceduralMeshComponent.h"
#include "Components/Rendering/LightComponent.h"
#include "Components/Rendering/MeshComponent.h"
#include "Components/Scripting/AngelScriptComponent.h"
#include "Core/Engine.h"
#include "ECS/EnTTSubsystem.h"
#include "misc/cpp/imgui_stdlib.h"

#include <string>

namespace puffin
{
	namespace ui
	{
		void UIWindowEntityProperties::draw(double dt)
		{
			mWindowName = "Entity Properties";

			//auto ecsWorld = m_engine->getSubsystem<ECS::World>();

			mEnTTSubsystem = mEngine->getSubsystem<ecs::EnTTSubsystem>();
			const auto registry = mEnTTSubsystem->registry();

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				begin(mWindowName);

				if (mSelectedEntity != gInvalidID && mEnTTSubsystem->valid(mSelectedEntity))
				{
					ImGui::Dummy(ImVec2(0.0f, 5.0f));
					ImGui::Text(""); ImGui::SameLine(0.0f);

					const auto entity = mEnTTSubsystem->getEntity(mSelectedEntity);
					auto object = registry->get<SceneObjectComponent>(entity);

					// Edit Entity Name
					std::string name = object.name;
					if (std::string* namePtr = &name; ImGui::InputText("##name", namePtr, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						object.name = *namePtr;
					}

					ImGui::Dummy(ImVec2(0.0f, 10.0f));
					ImGui::Text(" List of Components Here");

					// List of all Entity Components
					ImVec2 listBoxSize = ImGui::GetWindowSize();
					listBoxSize.y -= 110.0f;

					if (ImGui::BeginListBox("##EntityProperties", listBoxSize))
					{
						ImGuiTreeNodeFlags flags = 0;
						//flags |= ImGuiTreeNodeFlags_Framed;
						//flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
						flags |= ImGuiTreeNodeFlags_NoAutoOpenOnLog;
						flags |= ImGuiTreeNodeFlags_OpenOnArrow;
						flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
						flags |= ImGuiTreeNodeFlags_AllowItemOverlap;
						flags |= ImGuiTreeNodeFlags_DefaultOpen;
						flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

						mSceneChanged = false;

						// Display Component UI

						if (registry->any_of<TransformComponent>(entity))
						{
							auto& transform = registry->get<TransformComponent>(entity);

							drawTransformUI(flags, entity, transform);
						}

						if (registry->any_of<rendering::MeshComponent>(entity))
						{
							auto& mesh = registry->get<rendering::MeshComponent>(entity);

							drawMeshUI(flags, entity, mesh);
						}

						if (registry->any_of<rendering::LightComponent>(entity))
						{
							auto& light = registry->get<rendering::LightComponent>(entity);

							drawLightUI(flags, entity, light);
						}

						if (registry->any_of<rendering::ShadowCasterComponent>(entity))
						{
							auto& shadowcaster = registry->get<rendering::ShadowCasterComponent>(entity);

							drawShadowcasterUI(flags, entity, shadowcaster);
						}

						if (registry->any_of<procedural::PlaneComponent>(entity))
						{
							auto& plane = registry->get<procedural::PlaneComponent>(entity);

							drawProceduralPlaneUI(flags, entity, plane);
						}

						if (registry->any_of<physics::RigidbodyComponent2D>(entity))
						{
							auto& rigidbody = registry->get<physics::RigidbodyComponent2D>(entity);

							drawRigidbody2DUI(flags, entity, rigidbody);
						}

						if (registry->any_of<physics::CircleComponent2D>(entity))
						{
							auto& circle = registry->get<physics::CircleComponent2D>(entity);

							drawCircle2DUI(flags, entity, circle);
						}

						if (registry->any_of<physics::BoxComponent2D>(entity))
						{
							auto& box = registry->get<physics::BoxComponent2D>(entity);

							drawBox2DUI(flags, entity, box);
						}

						if (registry->any_of<scripting::AngelScriptComponent>(entity))
						{
							auto& script = registry->get<scripting::AngelScriptComponent>(entity);

							drawScriptUI(flags, entity, script);
						}

						ImGui::EndListBox();
					}

					ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2 - 50.0f, 0.0f)); ImGui::SameLine();

					if (ImGui::Button("Add Component"))
					{
						ImGui::OpenPopup("Add Component");
					}

					// Display Add Component Popup
					/*if (ImGui::BeginPopup("Add Component"))
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
							if (!ecsWorld->HasComponent<rendering::MeshComponent>(m_entity))
							{
								ecsWorld->AddComponent<rendering::MeshComponent>(m_entity);
								sceneChanged = true;
							}
						}

						ImGui::Separator();

						if (ImGui::Selectable("Light"))
						{
							ecsWorld->AddComponent<rendering::LightComponent>(m_entity);
						}

						if (ImGui::Selectable("Shadow Caster"))
						{
							ecsWorld->AddComponent<rendering::ShadowCasterComponent>(m_entity);
						}

						ImGui::Separator();
							
						if (ImGui::Selectable("Rigidbody"))
						{
							ecsWorld->AddComponent<physics::RigidbodyComponent2D>(m_entity);
						}

						if (ImGui::Selectable("Circle2D"))
						{
							ecsWorld->AddComponent<physics::CircleComponent2D>(m_entity);
						}

						if (ImGui::Selectable("Box2D"))
						{
							ecsWorld->AddComponent<physics::BoxComponent2D>(m_entity);
						}

						ImGui::EndPopup();
					}*/
				}
				else
				{
					ImGui::Text(" No Entity Selected");
				}

				end();
			}
		}

		void UIWindowEntityProperties::drawTransformUI(const ImGuiTreeNodeFlags flags, const entt::entity entity, TransformComponent& transform)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Transform Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Transform"))
				{
					mEnTTSubsystem->registry()->remove<TransformComponent>(entity);
					
					mSceneChanged = true;
				}

				{
#ifdef PFN_USE_DOUBLE_PRECISION

					Vector3d position = transform.position;

					if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &position, 3, 0.1f))
					{
						registry->patch<TransformComponent>(entity, [&position](auto& transform) { transform.position = position; });

						mSceneChanged = true;
					}

#else

					Vector3f position = transform.position;

					if (ImGui::DragFloat3("Position", reinterpret_cast<float*>(&position), 0.1f))
					{
						registry->patch<TransformComponent>(entity, [&position](auto& transform) { transform.position = position; });

						mSceneChanged = true;
					}

#endif
				}

				{
					maths::Quat orientation = transform.orientation;
					orientation.w = maths::radToDeg(orientation.w);

					if (ImGui::DragFloat3("Axis", reinterpret_cast<float*>(&orientation), 0.01f, -1.f, 1.f))
					{
						registry->patch<TransformComponent>(entity, [&orientation](auto& transform)
						{
							orientation.w = maths::degToRad(orientation.w);
							transform.orientation = orientation;
						});

						mSceneChanged = true;
					}

					if (ImGui::DragFloat("Angle", &orientation.w))
					{
						if (orientation.w > 180.0f)
						{
							orientation.w = -180.0f;
						}

						if (orientation.w < -180.0f)
						{
							orientation.w = 180.0f;
						}

						registry->patch<TransformComponent>(entity, [&orientation](auto& transform)
						{
							orientation.w = maths::degToRad(orientation.w);
							transform.orientation = orientation;
						});

						mSceneChanged = true;
					}
				}

				{
					Vector3f scale = transform.scale;

					if (ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&transform.scale), 0.1f))
					{
						registry->patch<TransformComponent>(entity, [&scale](auto& transform) { transform.scale = scale; });

						mSceneChanged = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowEntityProperties::drawMeshUI(const ImGuiTreeNodeFlags flags, const entt::entity entity, rendering::MeshComponent& mesh)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Mesh Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Mesh"))
				{
					mEnTTSubsystem->registry()->remove<rendering::MeshComponent>(entity);

					mSceneChanged = true;
				}

				ImGui::Text("Model UUID: %llu", mesh.meshAssetId);
				ImGui::Text("Texture UUID: %llu", mesh.textureAssetId);

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
					mSceneChanged = true;
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

		void UIWindowEntityProperties::drawLightUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::LightComponent& light)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Light Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Point Light"))
				{
					mEnTTSubsystem->registry()->remove<rendering::LightComponent>(entity);

					mSceneChanged = true;
				}

				int itemCurrentIdx = static_cast<int>(light.type);

				// Edit Light Type
				if (const char* comboLabel = rendering::gLightTypeLabels[itemCurrentIdx].c_str(); ImGui::BeginCombo("Type", comboLabel))
				{
					for (int i = 0; i < rendering::gLightTypeLabels.size(); i++)
					{
						const bool isSelected = (itemCurrentIdx == i);

						if (ImGui::Selectable(rendering::gLightTypeLabels[i].c_str(), isSelected))
						{
							itemCurrentIdx = i;

							const auto lightType = static_cast<rendering::LightType>(itemCurrentIdx);
							registry->patch<rendering::LightComponent>(entity, [&lightType](auto& light)
							{
								light.type = lightType;
							});

							mSceneChanged = true;
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				// Edit Light Diffuse Color
				{
					Vector3f color = light.color;

					if (ImGui::ColorEdit3("Diffuse", reinterpret_cast<float*>(&color)))
					{
						registry->patch<rendering::LightComponent>(entity, [&color](auto& light) { light.color = color; });

						mSceneChanged = true;
					}
				}

				// Edit Light Ambient Intensity
				{
					float ambientIntensity = light.ambientIntensity;

					if (ImGui::DragFloat("Ambient Intensity", &ambientIntensity, 0.001f, 0.0f, 1.0f))
					{
						registry->patch<rendering::LightComponent>(entity, [&ambientIntensity](auto& light) { light.ambientIntensity = ambientIntensity; });

						mSceneChanged = true;
					}
				}

				// Edit Light Specular Intensity
				{
					float specularIntensity = light.specularIntensity;

					if (ImGui::DragFloat("Specular Intensity", &specularIntensity, 0.001f, 0.0f, 1.0f))
					{
						registry->patch<rendering::LightComponent>(entity, [&specularIntensity](auto& light) { light.specularIntensity = specularIntensity; });

						mSceneChanged = true;
					}
				}

				// Edit Light Specular Exponent
				{
					int specularExponent = light.specularExponent;

					if (ImGui::DragInt("Specular Exponent", &specularExponent, 0.1f, 1, 128))
					{
						registry->patch<rendering::LightComponent>(entity, [&specularExponent](auto& light) { light.specularExponent = specularExponent; });

						mSceneChanged = true;
					}
				}

				if (light.type == rendering::LightType::Point || light.type == rendering::LightType::Spot)
				{
					{
						float linearAttenuation = light.linearAttenuation;

						if (ImGui::DragFloat("Linear Attenuation", &linearAttenuation, .01f, .01f, 1.f, "%.4f"))
						{
							registry->patch<rendering::LightComponent>(entity, [&linearAttenuation](auto& light) { light.linearAttenuation = linearAttenuation; });

							mSceneChanged = true;
						}
					}

					{
						float quadraticAttenuation = light.quadraticAttenuation;

						if (ImGui::DragFloat("Quadratic Attenuation", &quadraticAttenuation, .01f, .01f, 2.f, "%.6f"))
						{
							registry->patch<rendering::LightComponent>(entity, [&quadraticAttenuation](auto& light) { light.quadraticAttenuation = quadraticAttenuation; });

							mSceneChanged = true;
						}
					}
				}

				if (light.type == rendering::LightType::Spot)
				{
					{
						float innerCutoffAngle = light.innerCutoffAngle;

						if (ImGui::DragFloat("Inner Cutoff Angle", &innerCutoffAngle, 0.25f, 0.0f, 45.0f))
						{
							registry->patch<rendering::LightComponent>(entity, [&innerCutoffAngle](auto& light) { light.innerCutoffAngle = innerCutoffAngle; });

							mSceneChanged = true;
						}
					}

					// To avoid breaking the lighting, outerCutoffAngle should never be less than innerCutoffAngle
					{
						float outerCutoffAngle = light.outerCutoffAngle;

						if (ImGui::DragFloat("Outer Cutoff Angle", &outerCutoffAngle, 0.25f, light.outerCutoffAngle, 45.0f))
						{
							registry->patch<rendering::LightComponent>(entity, [&outerCutoffAngle](auto& light) { light.outerCutoffAngle = outerCutoffAngle; });

							mSceneChanged = true;
						}

						// Outer Cutoff will match inner cutoff if inner cutoff becomes larger
						if (light.outerCutoffAngle < light.innerCutoffAngle)
						{
							light.outerCutoffAngle = light.innerCutoffAngle;

							mSceneChanged = true;
						}
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowEntityProperties::drawShadowcasterUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::ShadowCasterComponent& shadowcaster)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Shadow Caster Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Shadow Caster"))
				{
					mEnTTSubsystem->registry()->remove<rendering::ShadowCasterComponent>(entity);

					mSceneChanged = true;
				}

				int itemCurrentIdx = 0;
				for (int i = 0; i < rendering::gShadowResolutionValues.size(); i++)
				{
					if (rendering::gShadowResolutionValues[i] == shadowcaster.width)
					{
						itemCurrentIdx = i;
						break;
					}
				}

				if (const char* label = rendering::gShadowResolutionLabels[itemCurrentIdx].c_str(); ImGui::BeginCombo("Shadow Resolution", label))
				{
					for (int i = 0; i < rendering::gShadowResolutionValues.size(); i++)
					{
						const bool isSelected = (itemCurrentIdx == i);

						if (ImGui::Selectable(rendering::gShadowResolutionLabels[i].c_str(), isSelected))
						{
							itemCurrentIdx = i;

							uint16_t width = rendering::gShadowResolutionValues[itemCurrentIdx];
							uint16_t height = width;
							
							registry->patch<rendering::ShadowCasterComponent>(entity, [&width, &height](auto& shadowcaster)
							{
								shadowcaster.width = width;
								shadowcaster.height = height;
							});

							mSceneChanged = true;
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				ImGui::TreePop();
			}
		}

		void UIWindowEntityProperties::drawProceduralPlaneUI(ImGuiTreeNodeFlags flags, entt::entity entity, procedural::PlaneComponent& plane)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Procedural Plane Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##ProceduralPlane"))
				{
					mEnTTSubsystem->registry()->remove<procedural::PlaneComponent>(entity);

					mSceneChanged = true;
				}

				{
					Vector2f halfSize = plane.halfSize;

					if (ImGui::DragFloat2("Half Size", reinterpret_cast<float*>(&halfSize), 0.1f))
					{
						registry->patch<procedural::PlaneComponent>(entity, [&halfSize](auto& plane) { plane.halfSize = halfSize; });

						mSceneChanged = true;
					}
				}

				{
					Vector2i numQuads = plane.numQuads;

					if (ImGui::DragInt2("Num Quads", reinterpret_cast<int*>(&numQuads)))
					{
						registry->patch<procedural::PlaneComponent>(entity, [&numQuads](auto& plane) { plane.numQuads = numQuads; });

						mSceneChanged = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowEntityProperties::drawRigidbody2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::RigidbodyComponent2D& rigidbody)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Rigidbody Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Rigidbody"))
				{
					mEnTTSubsystem->registry()->remove<physics::RigidbodyComponent2D>(entity);
						
					mSceneChanged = true;
				}

				// Combo box for Body Type Selection
				const char* items[] = { "Static", "Kinematic", "Dynamic" };
				int itemCurrentIdx = static_cast<int>(rigidbody.bodyType);

				if (const char* label = items[itemCurrentIdx]; ImGui::BeginCombo("Body Type", label))
				{
					for (int i = 0; i < IM_ARRAYSIZE(items); i++)
					{
						const bool isSelected = (itemCurrentIdx == i);
						if (ImGui::Selectable(items[i], isSelected))
						{
							itemCurrentIdx = i;

							const auto bodyType = static_cast<physics::BodyType>(itemCurrentIdx);

							registry->patch<physics::RigidbodyComponent2D>(entity, [&bodyType](auto& rigidbody)
							{
								rigidbody.bodyType = bodyType;
							});

							mSceneChanged = true;
						}

						if (isSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndCombo();
				}

				{
					float mass = rigidbody.mass;

					if (ImGui::DragFloat("Mass", &mass))
					{
						registry->patch<physics::RigidbodyComponent2D>(entity, [&mass](auto& rigidbody) { rigidbody.mass = mass; });

						mSceneChanged = true;
					}
				}

				{
					float elasticity = rigidbody.elasticity;

					if (ImGui::DragFloat("Elasticity", &elasticity))
					{
						registry->patch<physics::RigidbodyComponent2D>(entity, [&elasticity](auto& rigidbody) { rigidbody.elasticity = elasticity; });

						mSceneChanged = true;
					}
				}

				//ImGui::Checkbox("Sleeping Allowed", &sleep);
				//ImGui::Checkbox("Bullet", &bullet);
				//ImGui::Checkbox("Awake", &awake);
				//ImGui::Checkbox("Rotation Fixed", &fixedRotation);
				//ImGui::DragFloat("Angular Damping", &angularDamping);
				//ImGui::DragFloat("Linear Damping", &linearDamping);
				//ImGui::DragFloat("Gravity Scale", &gravityScale);

				ImGui::TreePop();
			}
		}

		void UIWindowEntityProperties::drawCircle2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::CircleComponent2D& circle)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Circle Component 2D", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Circle2D"))
				{
					mEnTTSubsystem->registry()->remove<physics::CircleComponent2D>(entity);

					mSceneChanged = true;
				}

				{
					Vector2f centreOfMass = circle.centreOfMass;

					if (ImGui::DragFloat2("Centre of Mass", reinterpret_cast<float*>(&centreOfMass)))
					{
						registry->patch<physics::CircleComponent2D>(entity, [&centreOfMass](auto& circle) { circle.centreOfMass = centreOfMass; });

						mSceneChanged = true;
					}
				}

				{
					float radius = circle.radius;

					if (ImGui::DragFloat("Radius", &radius))
					{
						registry->patch<physics::CircleComponent2D>(entity, [&radius](auto& circle) { circle.radius = radius; });

						mSceneChanged = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowEntityProperties::drawBox2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::BoxComponent2D& box)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Box Component 2D", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Box2D"))
				{
					mEnTTSubsystem->registry()->remove<physics::BoxComponent2D>(entity);					

					mSceneChanged = true;
				}

				{
					Vector2f centreOfMass = box.centreOfMass;

					if (ImGui::DragFloat2("Centre of Mass", reinterpret_cast<float*>(&centreOfMass)))
					{
						registry->patch<physics::BoxComponent2D>(entity, [&centreOfMass](auto& box) { box.centreOfMass = centreOfMass; });

						mSceneChanged = true;
					}
				}

				{
					Vector2f halfExtent = box.halfExtent;

					if (ImGui::DragFloat2("Half Extent", reinterpret_cast<float*>(&halfExtent), 0.1f, 0.0f))
					{
						registry->patch<physics::BoxComponent2D>(entity, [&halfExtent](auto& box) { box.halfExtent = halfExtent; });

						mSceneChanged = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowEntityProperties::drawScriptUI(ImGuiTreeNodeFlags flags, entt::entity entity, scripting::AngelScriptComponent& script)
		{
			const auto registry = mEnTTSubsystem->registry();

			if (ImGui::TreeNodeEx("Script Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Script"))
				{
					mEnTTSubsystem->registry()->remove<scripting::AngelScriptComponent>(entity);
					
					mSceneChanged = true;
				}

				ImGui::InputText("Script Name", &script.name);

				ImGui::Text("File Path:"); ImGui::SameLine(0.0f);
				if (ImGui::Selectable(script.dir.string().c_str(), false))
				{
					//fileDialog->Open();
					//modelSelected = true;
				}

				if (script.obj != 0)
				{
					ImGui::Separator();
					ImGui::Text("Editable Properties");

					// Display Variables in scripts marked as editable
					for (const int& index : script.editableProperties)
					{
						// Get Property Name/Type
						const char* propertyName = script.obj->GetPropertyName(index);
						const int propertyType = script.obj->GetPropertyTypeId(index);

						// Display property with correct UI based on its type
						if (propertyType == asTYPEID_BOOL)
						{
							auto boolProperty = static_cast<bool*>(script.obj->GetAddressOfProperty(index));

							ImGui::Checkbox(propertyName, boolProperty);
						}
						else if (propertyType == asTYPEID_INT32)
						{
							auto intProperty = static_cast<int*>(script.obj->GetAddressOfProperty(index));

							ImGui::InputInt(propertyName, intProperty);
						}
						else if (propertyType == asTYPEID_FLOAT)
						{
							auto* floatProperty = static_cast<float*>(script.obj->GetAddressOfProperty(index));

							ImGui::InputFloat(propertyName, floatProperty);
						}
						else if (propertyType == asTYPEID_DOUBLE)
						{
							auto* doubleProperty = static_cast<double*>(script.obj->GetAddressOfProperty(index));

							ImGui::InputDouble(propertyName, doubleProperty);
						}
						else
						{
							auto* stringProperty = static_cast<std::string*>(script.obj->GetAddressOfProperty(index));

							ImGui::InputText(propertyName, stringProperty);
						}
					}

					ImGui::Separator();
					ImGui::Text("Visible Properties");

					// Display Variables in scripts marked as visible
					for (const int& index : script.visibleProperties)
					{
						// Get Property Name/Type
						const char* propertyName = script.obj->GetPropertyName(index);
						const int propertyType = script.obj->GetPropertyTypeId(index);

						std::string propertyText = propertyName;

						// Display property with correct UI based on its type
						if (propertyType == asTYPEID_INT32)
						{
							const int intProperty = *static_cast<int*>(script.obj->GetAddressOfProperty(index));
							propertyText += ": %d";

							ImGui::Text(propertyText.c_str(), intProperty);
						}
						else if (propertyType == asTYPEID_FLOAT)
						{
							const float floatProperty = *static_cast<float*>(script.obj->GetAddressOfProperty(index));
							propertyText += ": %.20f";

							ImGui::Text(propertyText.c_str(), floatProperty);
						}
						else if (propertyType == asTYPEID_DOUBLE)
						{
							const double doubleProperty = *static_cast<double*>(script.obj->GetAddressOfProperty(index));
							propertyText += ": %.20f";

							ImGui::Text(propertyText.c_str(), doubleProperty);
						}
						else
						{
							std::string stringProperty = *static_cast<std::string*>(script.obj->GetAddressOfProperty(index));
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
