#include "puffin/editor/ui/windows/uiwindownodeeditor.h"

#include <string>

#include "puffin/core/engine.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/mathhelpers.h"
#include "misc/cpp/imgui_stdlib.h"
#include "puffin/scene/scenegraphsubsystem.h"
#include "puffin/nodes/node.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/components/physics/2d/rigidbodycomponent2d.h"
#include "puffin/components/physics/2d/shapecomponent2d.h"
#include "puffin/components/physics/2d/boxcomponent2d.h"
#include "puffin/components/physics/2d/circlecomponent2d.h"
#include "puffin/components/physics/3d/rigidbodycomponent3d.h"
#include "puffin/components/physics/3d/boxcomponent3d.h"
#include "puffin/components/physics/3d/spherecomponent3d.h"
#include "puffin/components/procedural/3d/proceduralplanecomponent3d.h"
#include "puffin/components/rendering/3d/staticmeshcomponent3d.h"
#include "puffin/components/rendering/3d/pointlightcomponent3d.h"
#include "puffin/components/rendering/3d/spotlightcomponent3d.h"
#include "puffin/components/rendering/3d/directionallightcomponent3d.h"
#include "puffin/components/rendering/3d/shadowcastercomponent3d.h"
#include "puffin/nodes/transformnode2d.h"
#include "puffin/nodes/transformnode3d.h"
#include "puffin/nodes/rendering/3d/pointlightnode3d.h"
#include "puffin/nodes/rendering/3d/spotlightnode3d.h"
#include "puffin/nodes/rendering/3d/directionallightnode3d.h"
#include "puffin/editor/ui/nodeeditoruihelpers.h"
#include "puffin/nodes/rendering/3d/staticmeshnode3d.h"

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
		}

		void UIWindowNodeEditor::Draw(double deltaTime)
		{
			mWindowName = "Entity Properties";

			const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto sceneGraphSubsystem = m_engine->GetSubsystem<scene::SceneGraphSubsystem>();

			const auto registry = enttSubsystem->GetRegistry();

			if (mShow)
			{
				ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);

				Begin(mWindowName);

				if (mSelectedEntity != gInvalidID && enttSubsystem->IsEntityValid(mSelectedEntity))
				{
					ImGui::Dummy(ImVec2(0.0f, 5.0f));
					ImGui::Text(""); ImGui::SameLine(0.0f);

					const auto entity = enttSubsystem->GetEntity(mSelectedEntity);
					const auto node = sceneGraphSubsystem->GetNode(mSelectedEntity);

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
						if (auto* transformNode2D = dynamic_cast<TransformNode2D*>(node); transformNode2D)
						{
							mSceneChanged |= DrawTransformNode2DUI(flags, transformNode2D);
						}

						if (auto* transformNode3D = dynamic_cast<TransformNode3D*>(node); transformNode3D)
						{
							mSceneChanged |= DrawTransformNode3DUI(flags, transformNode3D);
						}

						if (auto* staticMeshNode3D = dynamic_cast<rendering::StaticMeshNode3D*>(node); staticMeshNode3D)
						{
							mSceneChanged |= DrawStaticMeshNode3DUI(flags, staticMeshNode3D);
						}

						if (auto* lightNode = dynamic_cast<rendering::LightNode3D*>(node); lightNode)
						{
							mSceneChanged |= DrawLightNode3DUI(flags, lightNode);
						}

						if (auto* pointLightNode = dynamic_cast<rendering::PointLightNode3D*>(node); pointLightNode)
						{
							mSceneChanged |= DrawPointLightNode3DUI(flags, pointLightNode);
						}

						if (auto* spotLightNode = dynamic_cast<rendering::SpotLightNode3D*>(node); spotLightNode)
						{
							mSceneChanged |= DrawSpotLightNode3DUI(flags, spotLightNode);
						}

						if (auto* dirLightNode = dynamic_cast<rendering::DirectionalLightNode3D*>(node); dirLightNode)
						{
							mSceneChanged |= DrawDirLightNode3DUI(flags, dirLightNode);
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
						for (const auto& handler : mComponentHandlers)
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

				End();
			}
		}

		void UIWindowNodeEditor::SetFileBrowser(ImGui::FileBrowser* fileDialog)
		{
			mFileDialog = fileDialog;
		}

		bool UIWindowNodeEditor::GetSceneChanged() const
		{
			return mSceneChanged;
		}

		//void UIWindowNodeEditor::DrawPointLight3DUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::PointLightComponent3D& light)
		//{
		//	const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
		//	const auto registry = enttSubsystem->GetRegistry();

		//	if (ImGui::TreeNodeEx("Point Light Component", flags))
		//	{
		//		ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

		//		if (ImGui::SmallButton("X##Point Light"))
		//		{
		//			registry->remove<rendering::LightComponent3D>(entity);

		//			mSceneChanged = true;
		//		}

		//		// Edit Light Diffuse Color
		//		{
		//			Vector3f color = light.color;

		//			if (ImGui::ColorEdit3("Diffuse", reinterpret_cast<float*>(&color)))
		//			{
		//				registry->patch<rendering::LightComponent3D>(entity, [&color](auto& light) { light.color = color; });

		//				mSceneChanged = true;
		//			}
		//		}

		//		// Edit Light Ambient Intensity
		//		{
		//			float ambientIntensity = light.ambientIntensity;

		//			if (ImGui::DragFloat("Ambient Intensity", &ambientIntensity, 0.001f, 0.0f, 1.0f))
		//			{
		//				registry->patch<rendering::LightComponent3D>(entity, [&ambientIntensity](auto& light) { light.ambientIntensity = ambientIntensity; });

		//				mSceneChanged = true;
		//			}
		//		}

		//		// Edit Light Specular Intensity
		//		{
		//			float specularIntensity = light.specularIntensity;

		//			if (ImGui::DragFloat("Specular Intensity", &specularIntensity, 0.001f, 0.0f, 1.0f))
		//			{
		//				registry->patch<rendering::LightComponent3D>(entity, [&specularIntensity](auto& light) { light.specularIntensity = specularIntensity; });

		//				mSceneChanged = true;
		//			}
		//		}

		//		// Edit Light Specular Exponent
		//		{
		//			int specularExponent = light.specularExponent;

		//			if (ImGui::DragInt("Specular Exponent", &specularExponent, 0.1f, 1, 128))
		//			{
		//				registry->patch<rendering::LightComponent3D>(entity, [&specularExponent](auto& light) { light.specularExponent = specularExponent; });

		//				mSceneChanged = true;
		//			}
		//		}

		//		// Point Light UI
		//		if (registry->any_of<rendering::PointLightComponent3D>(entity))
		//		{
		//			auto& pointLight = registry->get<rendering::PointLightComponent3D>(entity);

		//			{
		//				float linearAttenuation = pointLight.linearAttenuation;

		//				if (ImGui::DragFloat("Linear Attenuation", &linearAttenuation, .01f, .01f, 1.f, "%.4f"))
		//				{
		//					registry->patch<rendering::PointLightComponent3D>(entity, [&linearAttenuation](auto& pointLight) { pointLight.linearAttenuation = linearAttenuation; });

		//					mSceneChanged = true;
		//				}
		//			}

		//			{
		//				float quadraticAttenuation = pointLight.quadraticAttenuation;

		//				if (ImGui::DragFloat("Quadratic Attenuation", &quadraticAttenuation, .01f, .01f, 2.f, "%.6f"))
		//				{
		//					registry->patch<rendering::PointLightComponent3D>(entity, [&quadraticAttenuation](auto& pointLight) { pointLight.quadraticAttenuation = quadraticAttenuation; });

		//					mSceneChanged = true;
		//				}
		//			}
		//		}

		//		// Spot Light UI
		//		if (registry->any_of<rendering::SpotLightComponent3D>(entity))
		//		{
		//			auto& spotLight = registry->get<rendering::SpotLightComponent3D>(entity);

		//			{
		//				float linearAttenuation = spotLight.linearAttenuation;

		//				if (ImGui::DragFloat("Linear Attenuation", &linearAttenuation, .01f, .01f, 1.f, "%.4f"))
		//				{
		//					registry->patch<rendering::SpotLightComponent3D>(entity, [&linearAttenuation](auto& spotLight) { spotLight.linearAttenuation = linearAttenuation; });

		//					mSceneChanged = true;
		//				}
		//			}

		//			{
		//				float quadraticAttenuation = spotLight.quadraticAttenuation;

		//				if (ImGui::DragFloat("Quadratic Attenuation", &quadraticAttenuation, .01f, .01f, 2.f, "%.6f"))
		//				{
		//					registry->patch<rendering::SpotLightComponent3D>(entity, [&quadraticAttenuation](auto& spotLight) { spotLight.quadraticAttenuation = quadraticAttenuation; });

		//					mSceneChanged = true;
		//				}
		//			}

		//			{
		//				float innerCutoffAngle = spotLight.innerCutoffAngle;

		//				if (ImGui::DragFloat("Inner Cutoff Angle", &innerCutoffAngle, 0.25f, 0.0f))
		//				{
		//					if (innerCutoffAngle > 90.0f)
		//						innerCutoffAngle = 90.0f;

		//					registry->patch<rendering::SpotLightComponent3D>(entity, [&innerCutoffAngle](auto& spotLight) { spotLight.innerCutoffAngle = innerCutoffAngle; });

		//					mSceneChanged = true;
		//				}
		//			}

		//			// To avoid breaking the lighting, outerCutoffAngle should never be less than innerCutoffAngle
		//			{
		//				float outerCutoffAngle = spotLight.outerCutoffAngle;

		//				if (ImGui::DragFloat("Outer Cutoff Angle", &outerCutoffAngle, 0.25f, spotLight.outerCutoffAngle))
		//				{
		//					if (outerCutoffAngle > 90.0f)
		//						outerCutoffAngle = 90.0f;

		//					registry->patch<rendering::SpotLightComponent3D>(entity, [&outerCutoffAngle](auto& spotLight) { spotLight.outerCutoffAngle = outerCutoffAngle; });

		//					mSceneChanged = true;
		//				}

		//				// Outer Cutoff will match inner cutoff if inner cutoff becomes larger
		//				if (spotLight.outerCutoffAngle < spotLight.innerCutoffAngle)
		//				{
		//					spotLight.outerCutoffAngle = spotLight.innerCutoffAngle;

		//					mSceneChanged = true;
		//				}
		//			}
		//		}

		//		ImGui::TreePop();
		//	}
		//}

		/*void UIWindowNodeEditor::DrawShadowCaster3DUI(ImGuiTreeNodeFlags flags, entt::entity entity, rendering::ShadowCasterComponent3D& shadow)
		{
			const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = enttSubsystem->GetRegistry();

			if (ImGui::TreeNodeEx("Shadow Caster Component", flags))
			{
				ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

				if (ImGui::SmallButton("X##Shadow Caster"))
				{
					enttSubsystem->GetRegistry()->remove<rendering::ShadowCasterComponent3D>(entity);

					mSceneChanged = true;
				}

				{
					int itemCurrentIdx = 0;
					for (int i = 0; i < rendering::gShadowResolutionValues.size(); i++)
					{
						if (rendering::gShadowResolutionValues[i] == shadow.width)
						{
							itemCurrentIdx = i;
							break;
						}
					}

					if (const char* label = rendering::gShadowResolutionLabels[itemCurrentIdx].c_str(); ImGui::BeginCombo("Shadow Resolution", label))
					{
						for (int i = 0; i < rendering::gShadowResolutionValues.size(); i++)
						{
							const bool selected = (itemCurrentIdx == i);

							if (ImGui::Selectable(rendering::gShadowResolutionLabels[i].c_str(), selected))
							{
								itemCurrentIdx = i;

								uint16_t width = rendering::gShadowResolutionValues[itemCurrentIdx];
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
		}*/

		//void UIWindowNodeEditor::DrawRigidbody2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::RigidbodyComponent2D& rigidbody)
		//{
		//	const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
		//	const auto registry = enttSubsystem->GetRegistry();

		//	if (ImGui::TreeNodeEx("Rigidbody Component", flags))
		//	{
		//		ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

		//		if (ImGui::SmallButton("X##Rigidbody"))
		//		{
		//			registry->remove<physics::RigidbodyComponent2D>(entity);
		//				
		//			mSceneChanged = true;
		//		}

		//		// Combo box for Body Type Selection
		//		const char* items[] = { "Static", "Kinematic", "Dynamic" };
		//		int itemCurrentIdx = static_cast<int>(rigidbody.bodyType);

		//		if (const char* label = items[itemCurrentIdx]; ImGui::BeginCombo("Body Type", label))
		//		{
		//			for (int i = 0; i < IM_ARRAYSIZE(items); i++)
		//			{
		//				const bool isSelected = (itemCurrentIdx == i);
		//				if (ImGui::Selectable(items[i], isSelected))
		//				{
		//					itemCurrentIdx = i;

		//					const auto bodyType = static_cast<physics::BodyType>(itemCurrentIdx);

		//					registry->patch<physics::RigidbodyComponent2D>(entity, [&bodyType](auto& rigidbody)
		//					{
		//						rigidbody.bodyType = bodyType;
		//					});

		//					mSceneChanged = true;
		//				}

		//				if (isSelected)
		//				{
		//					ImGui::SetItemDefaultFocus();
		//				}
		//			}

		//			ImGui::EndCombo();
		//		}

		//		{
		//			float mass = rigidbody.mass;

		//			if (ImGui::DragFloat("Mass", &mass))
		//			{
		//				registry->patch<physics::RigidbodyComponent2D>(entity, [&mass](auto& rigidbody) { rigidbody.mass = mass; });

		//				mSceneChanged = true;
		//			}
		//		}

		//		{
		//			float elasticity = rigidbody.elasticity;

		//			if (ImGui::DragFloat("Elasticity", &elasticity))
		//			{
		//				registry->patch<physics::RigidbodyComponent2D>(entity, [&elasticity](auto& rigidbody) { rigidbody.elasticity = elasticity; });

		//				mSceneChanged = true;
		//			}
		//		}

		//		//ImGui::Checkbox("Sleeping Allowed", &sleep);
		//		//ImGui::Checkbox("Bullet", &bullet);
		//		//ImGui::Checkbox("Awake", &awake);
		//		//ImGui::Checkbox("Rotation Fixed", &fixedRotation);
		//		//ImGui::DragFloat("Angular Damping", &angularDamping);
		//		//ImGui::DragFloat("Linear Damping", &linearDamping);
		//		//ImGui::DragFloat("Gravity Scale", &gravityScale);

		//		ImGui::TreePop();
		//	}
		//}

		//void UIWindowNodeEditor::DrawCircle2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::CircleComponent2D& circle)
		//{
		//	const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
		//	const auto registry = enttSubsystem->GetRegistry();

		//	if (ImGui::TreeNodeEx("Circle Component 2D", flags))
		//	{
		//		ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

		//		if (ImGui::SmallButton("X##Circle2D"))
		//		{
		//			registry->remove<physics::CircleComponent2D>(entity);

		//			mSceneChanged = true;
		//		}

		//		{
		//			Vector2f centreOfMass = circle.centreOfMass;

		//			if (ImGui::DragFloat2("Centre of Mass", reinterpret_cast<float*>(&centreOfMass)))
		//			{
		//				registry->patch<physics::CircleComponent2D>(entity, [&centreOfMass](auto& circle) { circle.centreOfMass = centreOfMass; });

		//				mSceneChanged = true;
		//			}
		//		}

		//		{
		//			float radius = circle.radius;

		//			if (ImGui::DragFloat("Radius", &radius))
		//			{
		//				registry->patch<physics::CircleComponent2D>(entity, [&radius](auto& circle) { circle.radius = radius; });

		//				mSceneChanged = true;
		//			}
		//		}

		//		ImGui::TreePop();
		//	}
		//}

		//void UIWindowNodeEditor::DrawBox2DUI(ImGuiTreeNodeFlags flags, entt::entity entity, physics::BoxComponent2D& box)
		//{
		//	const auto enttSubsystem = m_engine->GetSubsystem<ecs::EnTTSubsystem>();
		//	const auto registry = enttSubsystem->GetRegistry();

		//	if (ImGui::TreeNodeEx("Box Component 2D", flags))
		//	{
		//		ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

		//		if (ImGui::SmallButton("X##Box2D"))
		//		{
		//			registry->remove<physics::BoxComponent2D>(entity);					

		//			mSceneChanged = true;
		//		}

		//		{
		//			Vector2f centreOfMass = box.centreOfMass;

		//			if (ImGui::DragFloat2("Centre of Mass", reinterpret_cast<float*>(&centreOfMass)))
		//			{
		//				registry->patch<physics::BoxComponent2D>(entity, [&centreOfMass](auto& box) { box.centreOfMass = centreOfMass; });

		//				mSceneChanged = true;
		//			}
		//		}

		//		{
		//			Vector2f halfExtent = box.halfExtent;

		//			if (ImGui::DragFloat2("Half Extent", reinterpret_cast<float*>(&halfExtent), 0.1f, 0.0f))
		//			{
		//				registry->patch<physics::BoxComponent2D>(entity, [&halfExtent](auto& box) { box.halfExtent = halfExtent; });

		//				mSceneChanged = true;
		//			}
		//		}

		//		ImGui::TreePop();
		//	}
		//}
	}
}
