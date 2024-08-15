#pragma once

//#include <Rendering/Vulkan/DebugDraw.h>
#include "puffin/components/transformcomponent3d.h"

namespace puffin
{
	enum class GizmoMode
	{
		Translation = 0,
		Rotation = 1,
		Scale = 2
	};

	extern GizmoMode gizmoMode;

	//inline void DrawTranslationGizmo(std::shared_ptr<core::EventSubsystem> eventSubsystem, Vector3f position)
	//{
	//	float lineLength = 2.5f;

	//	//eventSubsystem->Publish<Debug::Line>(Debug::Line(position, position + Vector3f(0.0f, 0.0f, lineLength), Vector3f(1.0f, 0.0f, 0.0f)));
	//	//eventSubsystem->Publish<Debug::Line>(Debug::Line(position, position + Vector3f(0.0f, lineLength, 0.0f), Vector3f(0.0f, 1.0f, 0.0f)));
	//	//eventSubsystem->Publish<Debug::Line>(Debug::Line(position, position + Vector3f(lineLength, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 1.0f)));
	//}

	//inline void DrawRotationGizmo(std::shared_ptr<core::EventSubsystem> eventSubsystem, Vector3f position)
	//{

	//}

	//inline void DrawScaleGizmo(std::shared_ptr<core::EventSubsystem> eventSubsystem, Vector3f position)
	//{

	//}

	//inline void DrawManipulationGizmo(std::shared_ptr<core::EventSubsystem> eventSubsystem, TransformComponent3D transform)
	//{
	//	switch (gizmoMode)
	//	{
	//		case GizmoMode::Translation:
	//			DrawTranslationGizmo(eventSubsystem, transform.position);
	//			break;
	//		case GizmoMode::Rotation:
	//			DrawRotationGizmo(eventSubsystem, transform.position);
	//			break;
	//		case GizmoMode::Scale:
	//			DrawScaleGizmo(eventSubsystem, transform.position);
	//			break;
	//	}
	//}
}