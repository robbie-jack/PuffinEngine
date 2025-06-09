#include "ui/node_editor_ui_helpers.h"

#include "node/transform_2d_node.h"
#include "node/transform_3d_node.h"
#include "node/rendering/3d/static_mesh_3d_node.h"
#include "node/rendering/3d/light_3d_node.h"
#include "node/rendering/3d/point_light_3d_node.h"
#include "node/rendering/3d/spot_light_3d_node.h"
#include "node/rendering/3d/directional_light_3d_node.h"
#include "component/transform_component_2d.h"
#include "component/transform_component_3d.h"

namespace puffin::ui
{
	bool DrawTransformNode2DUI(ImGuiTreeNodeFlags flags, Transform2DNode* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Transform 2D", flags))
		{
			auto& transform = node->Transform();

			{
#ifdef PFN_DOUBLE_PRECISION

				if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &transform.position, 2, 0.1f))
				{
					nodeChanged |= true;
				}

#else

				if (ImGui::DragFloat2("Position", reinterpret_cast<float*>(&transform.position), 0.1f))
				{
					nodeChanged |= true;
				}

#endif
			}

			{
				if (ImGui::DragFloat("Rotation", &transform.rotation, 0.1f, -180.0f, 180.0f))
				{
					nodeChanged |= true;
				}
			}

			{
				if (ImGui::DragFloat2("Scale", reinterpret_cast<float*>(&transform.scale), 0.1f))
				{
					nodeChanged |= true;
				}
			}

			ImGui::TreePop();
		}

		return nodeChanged;
	}

	bool DrawTransformNode3DUI(ImGuiTreeNodeFlags flags, TransformNode3D* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Transform 3D", flags))
		{
			auto& transform = node->Transform();

			{
#ifdef PFN_USE_DOUBLE_PRECISION

				if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &transform.position, 3, 0.1f))
				{
					node->transform_3d().position = position;

					nodeChanged |= true;
				}

#else

				if (ImGui::DragFloat3("Position", reinterpret_cast<float*>(&transform.position), 0.1f))
				{
					nodeChanged |= true;
				}

#endif
			}

			{
				auto eulerAngles = transform.orientationEulerAngles;

				if (ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&eulerAngles), 0.2f, 0, 0, "%.3f"))
				{
					UpdateTransformOrientation(transform, eulerAngles);

					nodeChanged |= true;
				}
			}

			{
				if (ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&transform.scale), 0.1f))
				{
					nodeChanged |= true;
				}
			}

			ImGui::TreePop();
		}

		return nodeChanged;
	}

	bool DrawStaticMeshNode3DUI(ImGuiTreeNodeFlags flags, rendering::StaticMeshNode3D* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Static Mesh 3D", flags))
		{
			ImGui::SameLine(ImGui::GetWindowWidth() - 20.0f);

			ImGui::Text("Model UUID: %lu", node->GetMeshID());
			ImGui::Text("Material UUID: %lu", node->GetMaterialID());

			ImGui::TreePop();
		}

		return nodeChanged;
	}

	bool DrawLightNode3DUI(ImGuiTreeNodeFlags flags, rendering::LightNode3D* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Light", flags))
		{
			if (ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&node->Color())))
			{
				nodeChanged |= true;
			}

			if (ImGui::DragFloat("Ambient Intensity", &node->AmbientIntensity()))
			{
				nodeChanged |= true;
			}

			if (ImGui::DragFloat("Specular Intensity", &node->SpecularIntensity()))
			{
				nodeChanged |= true;
			}

			if (ImGui::DragInt("Specular Exponent", &node->SpecularExponent()))
			{
				nodeChanged |= true;
			}

			ImGui::TreePop();
		}

		return nodeChanged;
	}

	bool DrawPointLightNode3DUI(ImGuiTreeNodeFlags flags, puffin::rendering::PointLightNode3D* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Point Light", flags))
		{
			ImGui::TreePop();
		}

		return nodeChanged;
	}

	bool DrawSpotLightNode3DUI(ImGuiTreeNodeFlags flags, puffin::rendering::SpotLightNode3D* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Spot Light", flags))
		{
			ImGui::TreePop();
		}

		return nodeChanged;
	}

	bool DrawDirLightNode3DUI(ImGuiTreeNodeFlags flags, puffin::rendering::DirectionalLightNode3D* node)
	{
		bool nodeChanged = false;

		if (ImGui::TreeNodeEx("Directional Light", flags))
		{
			ImGui::TreePop();
		}

		return nodeChanged;
	}
}