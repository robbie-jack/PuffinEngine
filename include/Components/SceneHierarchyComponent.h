#pragma once

#include "Types/UUID.h"
#include "Components/TransformComponent2D.h"
#include "Components/TransformComponent3D.h"
#include "nlohmann/json.hpp"

namespace puffin
{
	struct SceneHierarchy
	{
		PuffinID parentID = gInvalidID; // ID of parent entity
		PuffinID firstChildID = gInvalidID; // ID of first child entity
		PuffinID nextSiblingID = gInvalidID; // ID of next sibling entity
	};

	struct SceneHierarchyComponent2D : SceneHierarchy
	{
		SceneHierarchyComponent2D() = default;

		TransformComponent2D globalTransform; // Global Transform of this entity

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SceneHierarchyComponent2D, parentID, firstChildID, nextSiblingID)
	};

	struct SceneHierarchyComponent3D : SceneHierarchy
	{
		SceneHierarchyComponent3D() = default;

		TransformComponent3D globalTransform; // Global Transform of this entity

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(SceneHierarchyComponent3D, parentID, firstChildID, nextSiblingID)
	};
}