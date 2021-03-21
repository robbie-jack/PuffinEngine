#pragma once

#include <Rendering\DebugDraw.h>
#include <Components\TransformComponent.h>
#include <ECS/ECS.h>

namespace Puffin
{
	enum class GizmoMode
	{
		Translation = 0,
		Rotation = 1,
		Scale = 2
	};

	extern GizmoMode gizmoMode;

	inline void DrawTranslationGizmo(std::shared_ptr<ECS::World> world, Vector3 position)
	{
		float lineLength = 2.5f;

		world->PublishEvent<Debug::Line>(Debug::Line(position, position + Vector3(0.0f, 0.0f, lineLength), Vector3(1.0f, 0.0f, 0.0f)));
		world->PublishEvent<Debug::Line>(Debug::Line(position, position + Vector3(0.0f, lineLength, 0.0f), Vector3(0.0f, 1.0f, 0.0f)));
		world->PublishEvent<Debug::Line>(Debug::Line(position, position + Vector3(lineLength, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f)));
	}

	inline void DrawRotationGizmo(std::shared_ptr<ECS::World> world, Vector3 position)
	{

	}

	inline void DrawScaleGizmo(std::shared_ptr<ECS::World> world, Vector3 position)
	{

	}

	inline void DrawManipulationGizmo(std::shared_ptr<ECS::World> world, TransformComponent transform)
	{
		switch (gizmoMode)
		{
			case GizmoMode::Translation:
				DrawTranslationGizmo(world, transform.position);
				break;
			case GizmoMode::Rotation:
				DrawRotationGizmo(world, transform.position);
				break;
			case GizmoMode::Scale:
				DrawScaleGizmo(world, transform.position);
				break;
		}
	}
}