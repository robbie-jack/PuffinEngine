#include "puffin/scene/scenegraphsubsystem.h"

#include "puffin/core/engine.h"
#include "puffin/ecs/enttsubsystem.h"
#include "puffin/nodes/transformnode2d.h"
#include "puffin/nodes/transformnode3d.h"
#include "puffin/nodes/rendering/3d/cameranode3d.h"
#include "puffin/nodes/rendering/3d/directionallightnode3d.h"
#include "puffin/nodes/rendering/3d/pointlightnode3d.h"
#include "puffin/nodes/rendering/3d/spotlightnode3d.h"
#include "puffin/nodes/rendering/3d/staticmeshnode3d.h"

namespace puffin::scene
{
	SceneGraphSubsystem::SceneGraphSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "SceneGraphSubsystem";
	}

	void SceneGraphSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		mSceneGraphUpdated = true;

		RegisterDefaultNodeTypes();
	}

	void SceneGraphSubsystem::EndPlay()
	{
		mNodeIDs.clear();
		mIDToType.clear();
		mRootNodeIDs.clear();
		mNodesToDestroy.clear();

		mGlobalTransform2Ds.clear();
		mGlobalTransform3Ds.clear();

		for (auto [type, node_array] : mNodeArrays)
		{
			node_array->Clear();
		}
	}

	void SceneGraphSubsystem::Update(double deltaTime)
	{
		UpdateSceneGraph();

		UpdateGlobalTransforms();
	}

	bool SceneGraphSubsystem::ShouldUpdate()
	{
		return true;
	}

	Node* SceneGraphSubsystem::AddNode(const char* typeName, UUID id)
	{
		return AddNodeInternal(typeName, id);
	}

	Node* SceneGraphSubsystem::AddChildNode(const char* typeName, UUID id, UUID parentID)
	{
		return AddNodeInternal(typeName, id, parentID);
	}

	Node* SceneGraphSubsystem::GetNode(const UUID& id)
	{
		if (!IsValidNode(id))
			return nullptr;

		return GetArray(mIDToType.at(id).c_str())->GetNodePtr(id);
	}

	bool SceneGraphSubsystem::IsValidNode(UUID id)
	{
		return mIDToType.find(id) != mIDToType.end();
	}

	const std::string& SceneGraphSubsystem::GetNodeTypeName(const UUID& id) const
	{
		return mIDToType.at(id);
	}

	const TransformComponent2D& SceneGraphSubsystem::GetNodeGlobalTransform2D(const UUID& id) const
	{
		return mGlobalTransform2Ds.at(id);
	}

	TransformComponent2D& SceneGraphSubsystem::GetNodeGlobalTransform2D(const UUID& id)
	{
		return mGlobalTransform2Ds.at(id);
	}

	const TransformComponent3D& SceneGraphSubsystem::GetNodeGlobalTransform3D(const UUID& id) const
	{
		return mGlobalTransform3Ds.at(id);
	}

	TransformComponent3D& SceneGraphSubsystem::GetNodeGlobalTransform3D(const UUID& id)
	{
		return mGlobalTransform3Ds.at(id);
	}

	void SceneGraphSubsystem::NotifyTransformChanged(UUID id)
	{
		mNodeTransformsToUpdate.insert(id);
	}

	void SceneGraphSubsystem::QueueDestroyNode(const UUID& id)
	{
		mNodesToDestroy.insert(id);
	}

	const std::vector<UUID>& SceneGraphSubsystem::GetNodeIDs() const
	{
		return mNodeIDs;
	}

	const std::vector<UUID>& SceneGraphSubsystem::GetRootNodeIDs() const
	{
		return mRootNodeIDs;
	}

	void SceneGraphSubsystem::RegisterDefaultNodeTypes()
	{
		RegisterNodeType<Node>();
		RegisterNodeType<TransformNode2D>();
		RegisterNodeType<TransformNode3D>();
		RegisterNodeType<rendering::StaticMeshNode3D>();
		RegisterNodeType<rendering::PointLightNode3D>();
		RegisterNodeType<rendering::SpotLightNode3D>();
		RegisterNodeType<rendering::DirectionalLightNode3D>();
		RegisterNodeType<rendering::CameraNode3D>();
	}

	void SceneGraphSubsystem::UpdateSceneGraph()
	{
		if (!mNodesToDestroy.empty())
		{
			for (const auto& id : mNodesToDestroy)
			{
				DestroyNode(id);
			}

			for (auto it = mRootNodeIDs.end(); it != mRootNodeIDs.begin(); --it)
			{
				if (mNodesToDestroy.count(*it) > 0)
					mRootNodeIDs.erase(it);
			}

			mNodesToDestroy.clear();
			mSceneGraphUpdated = true;
		}

		if (mSceneGraphUpdated)
		{
			mNodeIDs.clear();

			for (const auto& id : mRootNodeIDs)
			{
				AddIDAndChildIDs(id, mNodeIDs);
			}

			mSceneGraphUpdated = false;
		}
	}

	void SceneGraphSubsystem::DestroyNode(UUID id)
	{
		std::vector<UUID> childIDs;

		if (const auto node = GetNode(id); node)
		{
			node->EndPlay();

			node->GetChildIDs(childIDs);
		}

		GetArray(mIDToType.at(id).c_str())->RemoveNode(id);

		mIDToType.erase(id);

		if (mGlobalTransform2Ds.contains(id))
			mGlobalTransform2Ds.erase(id);

		if (mGlobalTransform3Ds.contains(id))
			mGlobalTransform3Ds.erase(id);

		for (const auto& childID : childIDs)
		{
			DestroyNode(childID);
		}
	}

	void SceneGraphSubsystem::AddIDAndChildIDs(UUID id, std::vector<UUID>& nodeIDs)
	{
		mNodeIDs.push_back(id);

		const auto node = GetNode(id);

		std::vector<UUID> childIDs;
		node->GetChildIDs(childIDs);

		for (const auto childID : childIDs)
		{
			AddIDAndChildIDs(childID, nodeIDs);
		}
	}

	void SceneGraphSubsystem::UpdateGlobalTransforms()
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->registry();

		mNodeTransformsAlreadyUpdated.clear();

		for (const auto& id : mNodeTransformsToUpdate)
		{
			UpdateGlobalTransform(id);
		}

		mNodeTransformsToUpdate.clear();
	}

	void SceneGraphSubsystem::UpdateGlobalTransform(UUID id)
	{
		if (const auto node = GetNode(id); node && mNodeTransformsAlreadyUpdated.find(id) == mNodeTransformsAlreadyUpdated.end())
		{
			const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto registry = enttSubsystem->registry();

			if (auto* transformNode2D = dynamic_cast<TransformNode2D*>(node))
			{
				auto& globalTransform = mGlobalTransform2Ds.at(id);
				globalTransform.position = { 0.f };
				globalTransform.rotation = 0.0f;
				globalTransform.scale = { 1.0f };

				std::vector<UUID> transformIDsToApply;
				transformIDsToApply.push_back(id);

				auto parentID = node->GetParentID();
				while (parentID != gInvalidID)
				{
					if (const auto parentNode = GetNode(parentID); parentNode)
					{
						transformIDsToApply.push_back(parentID);

						parentID = parentNode->GetParentID();
					}
				}

				for (int i = transformIDsToApply.size(); i-- > 0;)
				{
					const auto* transformNode = dynamic_cast<TransformNode2D*>(GetNode(transformIDsToApply[i]));

					ApplyLocalToGlobalTransform2D(transformNode->GetTransform(), globalTransform);
				}

				registry->patch<TransformComponent2D>(node->GetEntity());
			}

			if (auto* transformNode3D = dynamic_cast<TransformNode3D*>(node))
			{
				auto& globalTransform = mGlobalTransform3Ds.at(id);
				globalTransform.position = { 0.f };
				globalTransform.orientationQuat = angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0));
				globalTransform.orientationEulerAngles = { 0.0f, 0.0f, 0.0f };
				globalTransform.scale = { 1.f };

				auto parentID = node->GetParentID();

				std::vector<UUID> transformIDsToApply;
				transformIDsToApply.push_back(id);

				while (parentID != gInvalidID)
				{
					if (const auto parentNode = GetNode(parentID); parentNode)
					{
						transformIDsToApply.push_back(parentID);

						parentID = parentNode->GetParentID();
					}
				}

				for (int i = transformIDsToApply.size(); i-- > 0;)
				{
					const auto* transformNode = dynamic_cast<TransformNode3D*>(GetNode(transformIDsToApply[i]));

					ApplyLocalToGlobalTransform3D(transformNode->GetTransform(), globalTransform);
				}

				registry->patch<TransformComponent3D>(node->GetEntity());
			}

			// Make sure children also have the global transform updated as well
			std::vector<UUID> childIDs;
			node->GetChildIDs(childIDs);
			for (const auto& childID : childIDs)
			{
				UpdateGlobalTransform(childID);
			}

			mNodeTransformsAlreadyUpdated.insert(id);
		}
	}

	void SceneGraphSubsystem::ApplyLocalToGlobalTransform2D(const TransformComponent2D& localTransform, TransformComponent2D& globalTransform)
	{
		globalTransform.position += localTransform.position;
		globalTransform.rotation += localTransform.rotation;

		if (globalTransform.rotation > 360.0f)
			globalTransform.rotation -= 360.0f;

		globalTransform.scale *= localTransform.scale;
	}

	void SceneGraphSubsystem::ApplyLocalToGlobalTransform3D(const TransformComponent3D& localTransform, TransformComponent3D& globalTransform)
	{
		globalTransform.position += localTransform.position;

		globalTransform.orientationQuat = localTransform.orientationQuat * globalTransform.orientationQuat;

		globalTransform.orientationEulerAngles += localTransform.orientationEulerAngles;

		globalTransform.scale *= localTransform.scale;
	}

	void SceneGraphSubsystem::AddNodeInternalBase(Node* node, const char* typeName, UUID id, UUID parentID)
	{
		if (parentID != gInvalidID)
		{
			node->SetParentID(parentID);

			Node* parent_node_ptr = GetNode(parentID);
			parent_node_ptr->AddChildID(id);
		}
		else
		{
			mRootNodeIDs.push_back(id);
		}

		mIDToType.insert({ id, typeName });

		if (auto* transformNode2D = dynamic_cast<TransformNode2D*>(node))
		{
			mGlobalTransform2Ds.emplace(id, TransformComponent2D());
		}

		if (auto* transformNode3D = dynamic_cast<TransformNode3D*>(node))
		{
			mGlobalTransform3Ds.emplace(id, TransformComponent3D());
		}

		mSceneGraphUpdated = true;
	}
}
