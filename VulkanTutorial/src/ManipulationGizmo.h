#pragma once

#include <Rendering\DebugDraw.h>
#include <Components\TransformComponent.h>

namespace Puffin
{
	enum class GizmoMode
	{
		Translation = 0,
		Rotation = 1,
		Scale = 2
	};

	extern GizmoMode gizmoMode;

	inline void DrawTranslationGizmo(Vector3 position)
	{
		float lineLength = 2.5f;

		Debug::DrawLine(position, position + Vector3(0.0f, 0.0f, lineLength), Vector3(1.0f, 0.0f, 0.0f));
		Debug::DrawLine(position, position + Vector3(0.0f, lineLength, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
		Debug::DrawLine(position, position + Vector3(lineLength, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f));
	}

	inline void DrawRotationGizmo(Vector3 position)
	{

	}

	inline void DrawScaleGizmo(Vector3 position)
	{

	}

	inline void DrawManipulationGizmo(TransformComponent transform)
	{
		switch (gizmoMode)
		{
			case GizmoMode::Translation:
				DrawTranslationGizmo(transform.position);
				break;
			case GizmoMode::Rotation:
				DrawRotationGizmo(transform.position);
				break;
			case GizmoMode::Scale:
				DrawScaleGizmo(transform.position);
				break;
		}
	}
}