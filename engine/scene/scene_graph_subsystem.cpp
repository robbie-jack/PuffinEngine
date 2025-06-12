#include "scene/scene_graph_subsystem.h"

#include "core/engine.h"
#include "ecs/entt_subsystem.h"
#include "node/transform_2d_node.h"
#include "node/transform_3d_node.h"
#include "node/rendering/3d/camera_3d_node.h"
#include "node/rendering/3d/directional_light_3d_node.h"
#include "node/rendering/3d/point_light_3d_node.h"
#include "node/rendering/3d/spot_light_3d_node.h"
#include "node/rendering/3d/static_mesh_3d_node.h"
#include "component/transform_component_3d.h"
#include "node/physics/2d/box_2d_node.h"
#include "node/physics/2d/rigidbody_2d_node.h"
#include "node/rendering/2d/sprite_2d_node.h"
#include "rendering/render_globals.h"

namespace puffin::scene
{
	SceneGraphSubsystem::SceneGraphSubsystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "SceneGraphSubsystem";
	}

	void SceneGraphSubsystem::RegisterTypes()
	{
		RegisterNodeType<Node>();

		RegisterNodeType<Transform2DNode>();
		RegisterNodeType<rendering::Sprite2DNode>();
		RegisterNodeType<physics::Rigidbody2DNode>();
		RegisterNodeType<physics::Box2DNode>();

		RegisterNodeType<TransformNode3D>();
		RegisterNodeType<rendering::StaticMeshNode3D>();
		RegisterNodeType<rendering::PointLightNode3D>();
		RegisterNodeType<rendering::SpotLightNode3D>();
		RegisterNodeType<rendering::DirectionalLightNode3D>();
		RegisterNodeType<rendering::CameraNode3D>();

		SetDefaultNodePoolSize<Node>(5000);

		SetDefaultNodePoolSize<Transform2DNode>(5000);
		SetDefaultNodePoolSize<rendering::Sprite2DNode>(5000);
		SetDefaultNodePoolSize<physics::Rigidbody2DNode>(5000);
		SetDefaultNodePoolSize<physics::Box2DNode>(5000);

		SetDefaultNodePoolSize<TransformNode3D>(5000);
		SetDefaultNodePoolSize<rendering::StaticMeshNode3D>(5000);
		SetDefaultNodePoolSize<rendering::PointLightNode3D>(rendering::gMaxPointLights);
		SetDefaultNodePoolSize<rendering::SpotLightNode3D>(rendering::gMaxSpotLights);
		SetDefaultNodePoolSize<rendering::DirectionalLightNode3D>(rendering::gMaxDirectionalLights);
	}

	void SceneGraphSubsystem::Initialize(core::SubsystemManager* subsystemManager)
	{
		mSceneGraphUpdated = true;

		for (auto [typeID, nodePool] : mNodePools)
		{
			if (mNodePoolDefaultSizes.find(typeID) != mNodePoolDefaultSizes.end())
			{
				nodePool->Resize(mNodePoolDefaultSizes.at(typeID));
			}
			else
			{
				nodePool->Resize(gDefaultNodePoolSize);
			}
		}
	}

	void SceneGraphSubsystem::Deinitialize()
	{
		for (auto [typeID, nodePool] : mNodePools)
		{
			nodePool->Clear();
			delete nodePool;
		}

		mNodePools.clear();
	}

	void SceneGraphSubsystem::EndPlay()
	{
		mNodeIDs.clear();
		mIDToTypeID.clear();
		mRootNodeIDs.clear();
		mNodesToDestroy.clear();

		mGlobalTransform3Ds.Clear();

		for (auto [typeID, nodePool] : mNodePools)
		{
			nodePool->Reset();
		}
	}

	void SceneGraphSubsystem::Update(double deltaTime)
	{
		UpdateSceneGraph();

		//UpdateGlobalTransforms();
	}

	bool SceneGraphSubsystem::ShouldUpdate()
	{
		return true;
	}

	Node* SceneGraphSubsystem::AddNode(uint32_t typeID, const std::string& name, UUID id)
	{
		return AddNodeInternal(typeID, name, id);
	}

	Node* SceneGraphSubsystem::AddChildNode(uint32_t typeID, const std::string& name, UUID id, UUID parentID)
	{
		return AddNodeInternal(typeID, name, id, parentID);
	}

	Node* SceneGraphSubsystem::GetNode(const UUID& id) const
	{
		if (!IsValidNode(id))
			return nullptr;

		return GetPool(mIDToTypeID.at(id))->GetNode(id);
	}

	bool SceneGraphSubsystem::IsValidNode(UUID id) const
	{
		return mIDToTypeID.find(id) != mIDToTypeID.end();
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

		GetPool(mIDToTypeID.at(id))->RemoveNode(id);

		mIDToTypeID.erase(id);

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

	void SceneGraphSubsystem::AddNodeInternalBase(Node* node, uint32_t typeID, UUID id, UUID parentID)
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

		mIDToTypeID.insert({ id, typeID });

		if (auto* transformNode3D = dynamic_cast<TransformNode3D*>(node))
		{
			mGlobalTransform3Ds.Emplace(id, TransformComponent3D());

			NotifyTransformChanged(id);
		}

		mSceneGraphUpdated = true;
	}
}
