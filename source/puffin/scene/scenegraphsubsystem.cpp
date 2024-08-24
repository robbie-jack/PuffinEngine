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
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/transformcomponent3d.h"

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

		mGlobalTransform2Ds.Clear();
		mGlobalTransform3Ds.Clear();

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
		return mGlobalTransform2Ds.At(id);
	}

	TransformComponent2D& SceneGraphSubsystem::GetNodeGlobalTransform2D(const UUID& id)
	{
		return mGlobalTransform2Ds.At(id);
	}

	const TransformComponent3D& SceneGraphSubsystem::GetNodeGlobalTransform3D(const UUID& id) const
	{
		return mGlobalTransform3Ds.At(id);
	}

	TransformComponent3D& SceneGraphSubsystem::GetNodeGlobalTransform3D(const UUID& id)
	{
		return mGlobalTransform3Ds.At(id);
	}

	void SceneGraphSubsystem::NotifyTransformChanged(UUID id)
	{
		if (mNodeTransformsNeedUpdated.find(id) == mNodeTransformsNeedUpdated.end())
		{
			mNodeTransformsNeedUpdated.insert(id);
			mNodeTransformsNeedUpdatedVector.push_back(id);

			if (mNodeTransformsUpToDate.find(id) != mNodeTransformsUpToDate.end())
			{
				mNodeTransformsUpToDate.erase(id);
			}

			const auto node = GetNode(id);
			for (const auto& childID : node->GetChildIDs())
			{
				NotifyTransformChanged(childID);
			}
		}
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
		if (const auto node = GetNode(id); node)
		{
			for (const auto& childID : node->GetChildIDs())
			{
				DestroyNode(childID);
			}

			node->EndPlay();

			node->Deinitialize();
		}

		GetArray(mIDToType.at(id).c_str())->RemoveNode(id);

		mIDToType.erase(id);

		if (mGlobalTransform2Ds.Contains(id))
			mGlobalTransform2Ds.Erase(id);

		if (mGlobalTransform3Ds.Contains(id))
			mGlobalTransform3Ds.Erase(id);
	}

	void SceneGraphSubsystem::AddIDAndChildIDs(UUID id, std::vector<UUID>& nodeIDs)
	{
		mNodeIDs.push_back(id);

		const auto node = GetNode(id);

		for (const auto childID : node->GetChildIDs())
		{
			AddIDAndChildIDs(childID, nodeIDs);
		}
	}

	void SceneGraphSubsystem::UpdateGlobalTransforms()
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		for (const auto& id : mNodeTransformsNeedUpdatedVector)
		{
			UpdateGlobalTransform(id);
		}

		mNodeTransformsNeedUpdated.clear();
		mNodeTransformsNeedUpdatedVector.clear();
	}

	void SceneGraphSubsystem::UpdateGlobalTransform(UUID id)
	{
		if (mNodeTransformsUpToDate.find(id) == mNodeTransformsUpToDate.end())
		{
			if (const auto node = GetNode(id); node)
			{
				const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
				const auto registry = enttSubsystem->GetRegistry();

				if (auto* transformNode2D = dynamic_cast<TransformNode2D*>(node))
				{
					auto& globalTransform = mGlobalTransform2Ds.At(id);
					globalTransform.position = { 0.f };
					globalTransform.rotation = 0.0f;
					globalTransform.scale = { 1.0f };

					const auto& localTransform = transformNode2D->GetTransform();

					auto parentNode = dynamic_cast<TransformNode2D*>(node->GetParent());
					if (parentNode)
					{
						const auto& parentTransform = parentNode->GetGlobalTransform();

						ApplyLocalToGlobalTransform2D(localTransform, parentTransform, globalTransform);
					}
					else
					{
						ApplyLocalToGlobalTransform2D(localTransform, {}, globalTransform);
					}

					registry->patch<TransformComponent2D>(node->GetEntity());
				}

				if (auto* transformNode3D = dynamic_cast<TransformNode3D*>(node))
				{
					auto& globalTransform = mGlobalTransform3Ds.At(id);
					globalTransform.position = { 0.f };
					globalTransform.orientationQuat = angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0));
					globalTransform.orientationEulerAngles = { 0.0f, 0.0f, 0.0f };
					globalTransform.scale = { 1.f };

					const auto& localTransform = transformNode3D->GetTransform();

					auto parentNode = dynamic_cast<TransformNode3D*>(node->GetParent());
					if (parentNode)
					{
						const auto& parentTransform = parentNode->GetGlobalTransform();

						ApplyLocalToGlobalTransform3D(localTransform, parentTransform, globalTransform);
					}
					else
					{
						ApplyLocalToGlobalTransform3D(localTransform, {}, globalTransform);
					}

					registry->patch<TransformComponent3D>(node->GetEntity());
				}

				mNodeTransformsUpToDate.insert(id);
			}
		}
	}

	void SceneGraphSubsystem::LimitAngleTo180Degrees(float& angle)
	{
		constexpr float angleLimit = 180.0f;
		constexpr float angleChange = 360.0f;

		if (angle > angleLimit)
			angle -= angleChange;

		if (angle < -angleLimit)
			angle += angleChange;
	}

	void SceneGraphSubsystem::ApplyLocalToGlobalTransform2D(const TransformComponent2D& localTransform, const TransformComponent2D& globalTransform, TransformComponent2D
	                                                        & updatedTransform)
	{
		updatedTransform.position = globalTransform.position + localTransform.position;
		updatedTransform.rotation = globalTransform.rotation + localTransform.rotation;

		LimitAngleTo180Degrees(updatedTransform.rotation);

		updatedTransform.scale = globalTransform.scale * localTransform.scale;
	}

	void SceneGraphSubsystem::ApplyLocalToGlobalTransform3D(const TransformComponent3D& localTransform, const TransformComponent3D& globalTransform, TransformComponent3D&
	                                                        updatedTransform)
	{
		updatedTransform.position = globalTransform.position + localTransform.position;

		updatedTransform.orientationQuat = localTransform.orientationQuat * globalTransform.orientationQuat;

		updatedTransform.orientationEulerAngles = globalTransform.orientationEulerAngles + localTransform.orientationEulerAngles;

		LimitAngleTo180Degrees(updatedTransform.orientationEulerAngles.pitch);
		LimitAngleTo180Degrees(updatedTransform.orientationEulerAngles.yaw);
		LimitAngleTo180Degrees(updatedTransform.orientationEulerAngles.roll);

		updatedTransform.scale = globalTransform.scale * localTransform.scale;
	}

	void SceneGraphSubsystem::AddNodeInternalBase(Node* node, const char* typeName, UUID id, UUID parentID)
	{
		assert(node != nullptr && "SceneGraphSubsystem::AddNodeInternalBase - Node was nullptr");

		// Set node parent if necessary
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

		node->Initialize();

		mIDToType.insert({ id, typeName });

		if (auto* transformNode2D = dynamic_cast<TransformNode2D*>(node))
		{
			mGlobalTransform2Ds.Emplace(id, TransformComponent2D());

			NotifyTransformChanged(id);
		}

		if (auto* transformNode3D = dynamic_cast<TransformNode3D*>(node))
		{
			mGlobalTransform3Ds.Emplace(id, TransformComponent3D());

			NotifyTransformChanged(id);
		}

		mSceneGraphUpdated = true;
	}
}
