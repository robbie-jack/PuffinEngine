#include "puffin/editor/ui/windows/uiwindownodeeditor.h"

#include <string>

#include "puffin/core/engine.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/mathhelpers.h"
#include "misc/cpp/imgui_stdlib.h"
#include "puffin/scene/scenegraph.h"
#include "puffin/nodes/node.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/components/physics/2d/rigidbodycomponent2d.h"
#include "puffin/components/physics/2d/shapecomponent2d.h"
#include "puffin/components/physics/2d/boxcomponent2d.h"
#include "puffin/components/physics/2d/circlecomponent2d.h"
#include "puffin/components/physics/2d/velocitycomponent2d.h"
#include "puffin/components/physics/3d/rigidbodycomponent3d.h"
#include "puffin/components/physics/3d/shapecomponent3d.h"
#include "puffin/components/physics/3d/boxcomponent3d.h"
#include "puffin/components/physics/3d/spherecomponent3d.h"
#include "puffin/components/physics/3d/velocitycomponent3d.h"
#include "puffin/components/scripting/angelscriptcomponent.h"
#include "puffin/components/procedural/3d/proceduralplanecomponent3d.h"
#include "puffin/components/rendering/3d/staticmeshcomponent3d.h"
#include "puffin/components/rendering/3d/lightcomponent3d.h"
#include "puffin/components/rendering/3d/pointlightcomponent3d.h"
#include "puffin/components/rendering/3d/spotlightcomponent3d.h"
#include "puffin/components/rendering/3d/shadowcastercomponent3d.h"
#include "puffin/nodes/transformnode3d.h"

namespace puffin
{
	namespace ui
	{
		UIWindowNodeEditor::UIWindowNodeEditor(const std::shared_ptr<core::Engine>& engine) : UIWindow(engine)
		{
			AddComponentType<TransformComponent3D>("Transform3D");
			AddComponentType<rendering::StaticMeshComponent3D>("Static Mesh");
			AddComponentType<rendering::LightComponent3D>("Light 3D");
			AddComponentType<rendering::ShadowCasterComponent3D>("Shadow Caster");
			AddComponentType<physics::RigidbodyComponent3D>("Rigidbody3D");
			AddComponentType<physics::BoxComponent3D>("Box3D");
			AddComponentType<physics::SphereComponent3D>("Sphere3D");
			AddComponentType<scripting::AngelScriptComponent>("Script");
		}

		void UIWindowNodeEditor::draw(double dt)
		{
			mWindowName = "Entity Properties";

			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			auto scene_graph_subsystem = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();

			const auto registry = entt_subsystem->registry();

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				begin(mWindowName);

				if (mSelectedEntity != gInvalidID && entt_subsystem->valid(mSelectedEntity))
				{
					ImGui::Dummy(ImVec2(0.0f, 5.0f));
					ImGui::Text(""); ImGui::SameLine(0.0f);

					const auto entity = entt_subsystem->get_entity(mSelectedEntity);
					auto node = scene_graph_subsystem->GetNode(mSelectedEntity);

					// Edit Entity Name
					std::string name = node->GetName();
					if (std::string* namePtr = &name; ImGui::InputText("##name", namePtr, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						node->SetName(*namePtr);
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
						if (node->HasTransform2D())
						{
							DrawTransformUI2DNode(flags, node);
						}

						if (node->HasTransform3D())
						{
							DrawTransformUI3DNode(flags, node);
						}

						if (registry->any_of<rendering::StaticMeshComponent3D>(entity))
						{
							auto& mesh = registry->get<rendering::StaticMeshComponent3D>(entity);

							DrawMeshUI(flags, entity, mesh);
						}

						if (registry->any_of<rendering::LightComponent3D>(entity))
						{
							auto& light = registry->get<rendering::LightComponent3D>(entity);

							DrawLightUI(flags, entity, light);
						}

						if (registry->any_of<rendering::ShadowCasterComponent3D>(entity))
						{
							auto& shadowcaster = registry->get<rendering::ShadowCasterComponent3D>(entity);

							DrawShadowcasterUI(flags, entity, shadowcaster);
						}

						if (registry->any_of<procedural::ProceduralPlaneComponent3D>(entity))
						{
							auto& plane = registry->get<procedural::ProceduralPlaneComponent3D>(entity);

							DrawProceduralPlaneUI(flags, entity, plane);
						}

						if (registry->any_of<physics::RigidbodyComponent2D>(entity))
						{
							auto& rigidbody = registry->get<physics::RigidbodyComponent2D>(entity);

							DrawRigidbody2DUI(flags, entity, rigidbody);
						}

						if (registry->any_of<physics::CircleComponent2D>(entity))
						{
							auto& circle = registry->get<physics::CircleComponent2D>(entity);

							DrawCircle2DUI(flags, entity, circle);
						}

						if (registry->any_of<physics::BoxComponent2D>(entity))
						{
							auto& box = registry->get<physics::BoxComponent2D>(entity);

							DrawBox2DUI(flags, entity, box);
						}

						if (registry->any_of<scripting::AngelScriptComponent>(entity))
						{
							auto& script = registry->get<scripting::AngelScriptComponent>(entity);

							DrawScriptUI(flags, entity, script);
						}

						ImGui::EndListBox();
					}

					ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() / 2 - 50.0f, 0.0f)); ImGui::SameLine();

					if (ImGui::Button("Add Component"))
					{
						ImGui::OpenPopup("Add Component");
					}

					// Display Add Component Popup
					if (ImGui::BeginPopup("Add Component"))
					{
						for (auto& handler : mComponentHandlers)
						{
							if (ImGui::Selectable(handler->GetName().c_str()))
							{
								if (handler->Add(node))
								{
									mSceneChanged = true;
								}
							}
						}

						ImGui::EndPopup();
					}
				}
				else
				{
					ImGui::Text(" No Entity Selected");
				}

				end();
			}
		}

		void UIWindowNodeEditor::DrawTransformUI2DNode(ImGuiTreeNodeFlags flags, Node* node)
		{
			if (ImGui::TreeNodeEx("Transform", flags))
			{
                const auto transform = node->GetTransform2D();

				{
#ifdef PFN_DOUBLE_PRECISION

                    Vector2d position = transform->position;

					if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &position, 2, 0.1f))
					{
                        node->transform_2d()->position = position;

						mSceneChanged = true;
					}

#else

                    Vector2f position = transform->position;

					if (ImGui::DragFloat2("Position", reinterpret_cast<float*>(&position), 0.1f))
					{
                        node->GetTransform2D()->position = position;

						mSceneChanged = true;
					}

#endif
				}

				{
                    float rotation = transform->rotation;

					if (ImGui::DragFloat("Rotation", &rotation, 0.1f, -180.0f, 180.0f))
					{
                        node->GetTransform2D()->rotation = rotation;

						mSceneChanged = true;
					}
				}

				{
                    Vector2f scale = transform->scale;

					if (ImGui::DragFloat2("Scale", reinterpret_cast<float*>(&scale), 0.1f))
					{
                        node->GetTransform2D()->scale = scale;

						mSceneChanged = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::DrawTransformUI3DNode(ImGuiTreeNodeFlags flags, Node* node)
		{
			if (ImGui::TreeNodeEx("Transform", flags))
			{
				auto node_3d = static_cast<TransformNode3D*>(node);
                auto transform = node->GetTransform3D();

				{
#ifdef PFN_USE_DOUBLE_PRECISION

					Vector3d position = transform.position;

					if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &position, 3, 0.1f))
					{
						node->transform_3d().position = position;

						mSceneChanged = true;
					}

#else

                    Vector3f position = transform->position;

					if (ImGui::DragFloat3("Position", reinterpret_cast<float*>(&position), 0.1f))
					{
                        node->GetTransform3D()->position = position;

						mSceneChanged = true;
					}

#endif
				}

				{
					auto euler_angles = transform->orientationEulerAngles;

					if (ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&euler_angles), 0.2f, 0, 0, "%.3f"))
					{
						UpdateTransformOrientation(*node->GetTransform3D(), euler_angles);

						mSceneChanged = true;
					}
				}

				{
                    Vector3f scale = transform->scale;

					if (ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&scale), 0.1f))
					{
                        node->GetTransform3D()->scale = scale;

						mSceneChanged = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::DrawMeshUI(const ImGuiTreeNodeFlags flags, const entt::entity entity, rendering::StaticMeshComponent3D& mesh)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Mesh Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Mesh"))
				{
					entt_subsystem->registry()->remove<rendering::StaticMeshComponent3D>(entity);

					mSceneChanged = true;
				}

                ImGui::Text("Model UUID: %lu", mesh.meshID);
                ImGui::Text("Material UUID: %lu", mesh.materialID);

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

		void UIWindowNodeEditor::DrawLightUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::LightComponent3D& light)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Light Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Point Light"))
				{
					registry->remove<rendering::LightComponent3D>(entity);

					mSceneChanged = true;
				}

				// Edit Light Diffuse Color
				{
					Vector3f color = light.color;

					if (ImGui::ColorEdit3("Diffuse", reinterpret_cast<float*>(&color)))
					{
						registry->patch<rendering::LightComponent3D>(entity, [&color](auto& light) { light.color = color; });

						mSceneChanged = true;
					}
				}

				// Edit Light Ambient Intensity
				{
					float ambientIntensity = light.ambientIntensity;

					if (ImGui::DragFloat("Ambient Intensity", &ambientIntensity, 0.001f, 0.0f, 1.0f))
					{
						registry->patch<rendering::LightComponent3D>(entity, [&ambientIntensity](auto& light) { light.ambientIntensity = ambientIntensity; });

						mSceneChanged = true;
					}
				}

				// Edit Light Specular Intensity
				{
					float specularIntensity = light.specularIntensity;

					if (ImGui::DragFloat("Specular Intensity", &specularIntensity, 0.001f, 0.0f, 1.0f))
					{
						registry->patch<rendering::LightComponent3D>(entity, [&specularIntensity](auto& light) { light.specularIntensity = specularIntensity; });

						mSceneChanged = true;
					}
				}

				// Edit Light Specular Exponent
				{
					int specularExponent = light.specularExponent;

					if (ImGui::DragInt("Specular Exponent", &specularExponent, 0.1f, 1, 128))
					{
						registry->patch<rendering::LightComponent3D>(entity, [&specularExponent](auto& light) { light.specularExponent = specularExponent; });

						mSceneChanged = true;
					}
				}

				// Point Light UI
				if (registry->any_of<rendering::PointLightComponent3D>(entity))
				{
					auto& pointLight = registry->get<rendering::PointLightComponent3D>(entity);

					{
						float linearAttenuation = pointLight.linearAttenuation;

						if (ImGui::DragFloat("Linear Attenuation", &linearAttenuation, .01f, .01f, 1.f, "%.4f"))
						{
							registry->patch<rendering::PointLightComponent3D>(entity, [&linearAttenuation](auto& pointLight) { pointLight.linearAttenuation = linearAttenuation; });

							mSceneChanged = true;
						}
					}

					{
						float quadraticAttenuation = pointLight.quadraticAttenuation;

						if (ImGui::DragFloat("Quadratic Attenuation", &quadraticAttenuation, .01f, .01f, 2.f, "%.6f"))
						{
							registry->patch<rendering::PointLightComponent3D>(entity, [&quadraticAttenuation](auto& pointLight) { pointLight.quadraticAttenuation = quadraticAttenuation; });

							mSceneChanged = true;
						}
					}
				}

				// Spot Light UI
				if (registry->any_of<rendering::SpotLightComponent3D>(entity))
				{
					auto& spotLight = registry->get<rendering::SpotLightComponent3D>(entity);

					{
						float linearAttenuation = spotLight.linearAttenuation;

						if (ImGui::DragFloat("Linear Attenuation", &linearAttenuation, .01f, .01f, 1.f, "%.4f"))
						{
							registry->patch<rendering::SpotLightComponent3D>(entity, [&linearAttenuation](auto& spotLight) { spotLight.linearAttenuation = linearAttenuation; });

							mSceneChanged = true;
						}
					}

					{
						float quadraticAttenuation = spotLight.quadraticAttenuation;

						if (ImGui::DragFloat("Quadratic Attenuation", &quadraticAttenuation, .01f, .01f, 2.f, "%.6f"))
						{
							registry->patch<rendering::SpotLightComponent3D>(entity, [&quadraticAttenuation](auto& spotLight) { spotLight.quadraticAttenuation = quadraticAttenuation; });

							mSceneChanged = true;
						}
					}

					{
						float innerCutoffAngle = spotLight.innerCutoffAngle;

						if (ImGui::DragFloat("Inner Cutoff Angle", &innerCutoffAngle, 0.25f, 0.0f))
						{
							if (innerCutoffAngle > 90.0f)
								innerCutoffAngle = 90.0f;

							registry->patch<rendering::SpotLightComponent3D>(entity, [&innerCutoffAngle](auto& spotLight) { spotLight.innerCutoffAngle = innerCutoffAngle; });

							mSceneChanged = true;
						}
					}

					// To avoid breaking the lighting, outerCutoffAngle should never be less than innerCutoffAngle
					{
						float outerCutoffAngle = spotLight.outerCutoffAngle;

						if (ImGui::DragFloat("Outer Cutoff Angle", &outerCutoffAngle, 0.25f, spotLight.outerCutoffAngle))
						{
							if (outerCutoffAngle > 90.0f)
								outerCutoffAngle = 90.0f;

							registry->patch<rendering::SpotLightComponent3D>(entity, [&outerCutoffAngle](auto& spotLight) { spotLight.outerCutoffAngle = outerCutoffAngle; });

							mSceneChanged = true;
						}

						// Outer Cutoff will match inner cutoff if inner cutoff becomes larger
						if (spotLight.outerCutoffAngle < spotLight.innerCutoffAngle)
						{
							spotLight.outerCutoffAngle = spotLight.innerCutoffAngle;

							mSceneChanged = true;
						}
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::DrawShadowcasterUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::ShadowCasterComponent3D& shadow)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Shadow Caster Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Shadow Caster"))
				{
					entt_subsystem->registry()->remove<rendering::ShadowCasterComponent3D>(entity);

					mSceneChanged = true;
				}

				{
					int item_current_idx = 0;
					for (int i = 0; i < rendering::gShadowResolutionValues.size(); i++)
					{
						if (rendering::gShadowResolutionValues[i] == shadow.width)
						{
							item_current_idx = i;
							break;
						}
					}

					if (const char* label = rendering::gShadowResolutionLabels[item_current_idx].c_str(); ImGui::BeginCombo("Shadow Resolution", label))
					{
						for (int i = 0; i < rendering::gShadowResolutionValues.size(); i++)
						{
							const bool selected = (item_current_idx == i);

							if (ImGui::Selectable(rendering::gShadowResolutionLabels[i].c_str(), selected))
							{
								item_current_idx = i;

								uint16_t width = rendering::gShadowResolutionValues[item_current_idx];
								uint16_t height = width;

								registry->patch<rendering::ShadowCasterComponent3D>(entity, [&width, &height](auto& shadow)
								{
									shadow.width = width;
									shadow.height = height;
								});

								mSceneChanged = true;
							}

							if (selected)
								ImGui::SetItemDefaultFocus();
						}

						ImGui::EndCombo();
					}
				}

				{
					float biasMin = shadow.biasMin;

					if (ImGui::DragFloat("Bias Min", &biasMin, 0.001f))
					{
						registry->patch<rendering::ShadowCasterComponent3D>(entity, [&biasMin](auto& shadow)
						{
							shadow.biasMin = biasMin;
						});

						mSceneChanged = true;
					}
				}

				{
					float biasMax = shadow.biasMax;

					if (ImGui::DragFloat("Bias Max", &biasMax, 0.001f))
					{
						registry->patch<rendering::ShadowCasterComponent3D>(entity, [&biasMax](auto& shadow)
						{
							shadow.biasMax = biasMax;
						});

						mSceneChanged = true;
					}
				}

				{
					float boundsMult = shadow.boundsMult;

					if (ImGui::DragFloat("Bounds Multiplier", &boundsMult, 0.1f))
					{
						registry->patch<rendering::ShadowCasterComponent3D>(entity, [&boundsMult](auto& shadow)
						{
							shadow.boundsMult = boundsMult;
						});

						mSceneChanged = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::DrawProceduralPlaneUI(ImGuiTreeNodeFlags flags, entt::entity entity,
			procedural::ProceduralPlaneComponent3D& plane)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Procedural Plane Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##ProceduralPlane"))
				{
					registry->remove<procedural::ProceduralPlaneComponent3D>(entity);

					mSceneChanged = true;
				}

				{
					Vector2f halfSize = plane.halfSize;

					if (ImGui::DragFloat2("Half Size", reinterpret_cast<float*>(&halfSize), 0.1f))
					{
						registry->patch<procedural::ProceduralPlaneComponent3D>(entity, [&halfSize](auto& plane) { plane.halfSize = halfSize; });

						mSceneChanged = true;
					}
				}

				{
					Vector2i quadCount = plane.quadCount;

					if (ImGui::DragInt2("Num Quads", reinterpret_cast<int*>(&quadCount)))
					{
						registry->patch<procedural::ProceduralPlaneComponent3D>(entity, [&quadCount](auto& plane) { plane.quadCount = quadCount; });

						mSceneChanged = true;
					}
				}

				ImGui::TreePop();
			}
		}

		void UIWindowNodeEditor::DrawRigidbody2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::RigidbodyComponent2D& rigidbody)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Rigidbody Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Rigidbody"))
				{
					registry->remove<physics::RigidbodyComponent2D>(entity);
						
					mSceneChanged = true;
				}

				// Combo box for Body Type Selection
				const char* items[] = { "Static", "Kinematic", "Dynamic" };
				int item_current_idx = static_cast<int>(rigidbody.bodyType);

				if (const char* label = items[item_current_idx]; ImGui::BeginCombo("Body Type", label))
				{
					for (int i = 0; i < IM_ARRAYSIZE(items); i++)
					{
						const bool is_selected = (item_current_idx == i);
						if (ImGui::Selectable(items[i], is_selected))
						{
							item_current_idx = i;

							const auto bodyType = static_cast<physics::BodyType>(item_current_idx);

							registry->patch<physics::RigidbodyComponent2D>(entity, [&bodyType](auto& rigidbody)
							{
								rigidbody.bodyType = bodyType;
							});

							mSceneChanged = true;
						}

						if (is_selected)
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

		void UIWindowNodeEditor::DrawCircle2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::CircleComponent2D& circle)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Circle Component 2D", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Circle2D"))
				{
					registry->remove<physics::CircleComponent2D>(entity);

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

		void UIWindowNodeEditor::DrawBox2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::BoxComponent2D& box)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Box Component 2D", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Box2D"))
				{
					registry->remove<physics::BoxComponent2D>(entity);					

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

		void UIWindowNodeEditor::DrawScriptUI(ImGuiTreeNodeFlags flags, entt::entity entity, scripting::AngelScriptComponent& script)
		{
			auto entt_subsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = entt_subsystem->registry();

			if (ImGui::TreeNodeEx("Script Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Script"))
				{
					registry->remove<scripting::AngelScriptComponent>(entity);
					
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
