#pragma once

#include "imgui.h"

namespace puffin
{
	namespace rendering
	{
		class LightNode3D;
		class DirectionalLightNode3D;
		class SpotLightNode3D;
		class PointLightNode3D;
		class StaticMeshNode3D;
	}

	class TransformNode3D;
	class Transform2DNode;
}

namespace puffin::ui
{
	bool DrawTransformNode2DUI(ImGuiTreeNodeFlags flags, Transform2DNode* node);
	bool DrawTransformNode3DUI(ImGuiTreeNodeFlags flags, TransformNode3D* node);

	bool DrawStaticMeshNode3DUI(ImGuiTreeNodeFlags flags, rendering::StaticMeshNode3D* node);

	bool DrawLightNode3DUI(ImGuiTreeNodeFlags flags, rendering::LightNode3D* node);
	bool DrawPointLightNode3DUI(ImGuiTreeNodeFlags flags, rendering::PointLightNode3D* node);
	bool DrawSpotLightNode3DUI(ImGuiTreeNodeFlags flags, rendering::SpotLightNode3D* node);
	bool DrawDirLightNode3DUI(ImGuiTreeNodeFlags flags, rendering::DirectionalLightNode3D* node);
}