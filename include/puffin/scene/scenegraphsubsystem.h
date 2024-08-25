#pragma once

#include <cassert>
#include <unordered_map>
#include <memory>
#include <unordered_set>

#include "puffin/core/subsystem.h"
#include "puffin/nodes/node.h"
#include "puffin/types/uuid.h"
#include "puffin/types/storage/mappedarray.h"
#include "puffin/types/storage/mappedvector.h"

namespace puffin
{
	struct TransformComponent3D;
	struct TransformComponent2D;
}

namespace puffin
{
	namespace core
	{
		class Engine;
	}
}

namespace puffin::scene
{
	constexpr uint32_t gDefaultNodePoolSize = 5000;

	class INodePool
	{
	public:

		INodePool() = default;

		virtual ~INodePool() = default;

		virtual Node* AddNodePtr(const std::shared_ptr<core::Engine>& engine, const std::string& name, UUID id = gInvalidID) = 0;
		virtual Node* GetNodePtr(UUID id) = 0;
		virtual void RemoveNode(UUID id) = 0;
		virtual bool IsValid(UUID id) = 0;
		virtual void Reset() = 0;
		virtual void Clear() = 0;

	};

	template<typename T>
	class NodePool final : public INodePool
	{
	public:

		explicit NodePool(uint32_t poolSize)
		{
			mVector.Resize(poolSize);
		}

		~NodePool() override = default;

		T* AddNode(const std::shared_ptr<core::Engine>& engine, const std::string& name, UUID id = gInvalidID)
		{
			if (id == gInvalidID)
				id = GenerateId();

			if (mVector.Full())
			{
				mVector.Resize(mVector.Size() * 2);
			}

			mVector.Emplace(id, T{});

			T& node = mVector.At(id);
			auto* nodePtr = static_cast<Node*>(&node);
			nodePtr->Prepare(engine, name, id);

			return &node;
		}

		Node* AddNodePtr(const std::shared_ptr<core::Engine>& engine, const std::string& name, UUID id = gInvalidID) override
		{
			return static_cast<Node*>(AddNode(engine, name, id));
		}

		T* GetNode(UUID id)
		{
			return &mVector.At(id);
		}

		Node* GetNodePtr(UUID id) override
		{
			if (IsValid(id))
				return static_cast<Node*>(GetNode(id));

			return nullptr;
		}

		void RemoveNode(UUID id) override
		{
			GetNodePtr(id)->Reset();

			mVector.Erase(id, false);
		}

		bool IsValid(UUID id) override
		{
			return mVector.Contains(id);
		}

		/*
		 * Reset all nodes in pool
		 */
		void Reset() override
		{
			for (auto& node : mVector)
			{
				node.Reset();
			}

			mVector.Clear(false);
		}

		/*
		 * Reset & clear all nodes in pool
		 */
		void Clear() override
		{
			for (auto& node : mVector)
			{
				node.Reset();
			}

			mVector.Clear();
		}

	private:

		MappedVector<UUID, T> mVector;

	};

	class SceneGraphSubsystem : public core::Subsystem
	{
	public:

		explicit SceneGraphSubsystem(const std::shared_ptr<core::Engine>& engine);
		~SceneGraphSubsystem() override = default;

		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		void EndPlay() override;

		void Update(double deltaTime) override;
		bool ShouldUpdate() override;

		Node* AddNode(uint32_t typeID, const std::string& name, UUID id);
		Node* AddChildNode(uint32_t typeID, const std::string& name, UUID id, UUID parentID);
		[[nodiscard]] Node* GetNode(const UUID& id);
		bool IsValidNode(UUID id);

		[[nodiscard]] const TransformComponent2D& GetNodeGlobalTransform2D(const UUID& id) const;
		[[nodiscard]] TransformComponent2D& GetNodeGlobalTransform2D(const UUID& id);

		[[nodiscard]] const TransformComponent3D& GetNodeGlobalTransform3D(const UUID& id) const;
		[[nodiscard]] TransformComponent3D& GetNodeGlobalTransform3D(const UUID& id);

		void NotifyTransformChanged(UUID id);

		// Queue a node for destruction, will also destroy all child nodes
		void QueueDestroyNode(const UUID& id);

		[[nodiscard]] const std::vector<UUID>& GetNodeIDs() const;
		[[nodiscard]] const std::vector<UUID>& GetRootNodeIDs() const;

		template<typename T>
		void RegisterNodeType(uint32_t defaultNodePoolSize = gDefaultNodePoolSize)
		{
			auto type = entt::resolve<T>();
			const auto& typeID = type.id();

			if (mNodePools.find(typeID) == mNodePools.end())
			{
				mNodePools.insert({ typeID, static_cast<INodePool*>(new NodePool<T>(defaultNodePoolSize)) });
			}
		}

		template<typename T>
		T* AddNode(const std::string& name)
		{
			return AddNodeInternal<T>(name);
		}

		template<typename T>
		T* AddNode(const std::string& name, UUID id)
		{
			return AddNodeInternal<T>(name, id);
		}

		template<typename T>
		T* AddChildNode(const std::string& name, UUID parent_id)
		{
			return AddNodeInternal<T>(name, gInvalidID, parent_id);
		}

		template<typename T>
		T* AddChildNode(const std::string& name, UUID id, UUID parent_id)
		{
			return AddNodeInternal<T>(name, id, parent_id);
		}

		template<typename T>
		T* GetNode(UUID id)
		{
			if (!IsValidNode(id))
				return nullptr;

			return GetArray<T>()->GetNode(id);
		}

	private:

		void RegisterDefaultNodeTypes();

		void UpdateSceneGraph();
		void DestroyNode(UUID id);
		void AddIDAndChildIDs(UUID id, std::vector<UUID>& nodeIDs);

		void UpdateGlobalTransforms();
		void UpdateGlobalTransform(UUID id);

		static void LimitAngleTo180Degrees(float& angle);
		static void ApplyLocalToGlobalTransform2D(const TransformComponent2D& localTransform, const TransformComponent2D& globalTransform, TransformComponent2D
		                                          & updatedTransform);
		static void ApplyLocalToGlobalTransform3D(const TransformComponent3D& localTransform, const TransformComponent3D& globalTransform, TransformComponent3D&
		                                          updatedTransform);

		void AddNodeInternalBase(Node* node, uint32_t typeID, UUID id = gInvalidID, UUID parentID = gInvalidID);

		template<typename T>
		T* AddNodeInternal(const std::string& name, UUID id = gInvalidID, UUID parent_id = gInvalidID)
		{
			if (mIDToTypeID.find(id) != mIDToTypeID.end())
			{
				return GetArray<T>()->GetNode(id);
			}

			auto type = entt::resolve<T>();
			const auto& typeID = type.id();

			if (mNodePools.find(typeID) == mNodePools.end())
			{
				RegisterNodeType<T>();
			}

			Node* nodePtr;

			if (id == gInvalidID)
			{
				T* node = GetArray<T>()->AddNode(mEngine, name);
				nodePtr = static_cast<Node*>(node);

				id = nodePtr->GetID();
			}
			else
			{
				T* node = GetArray<T>()->AddNode(mEngine, name, id);
				nodePtr = static_cast<Node*>(node);
			}

			AddNodeInternalBase(nodePtr, typeID, id, parent_id);

			return GetArray<T>()->GetNode(id);
		}

		Node* AddNodeInternal(uint32_t typeID, const std::string& name, UUID id = gInvalidID, UUID parentID = gInvalidID)
		{
			if (mIDToTypeID.find(id) != mIDToTypeID.end())
			{
				return GetArray(typeID)->GetNodePtr(id);
			}


			assert(mNodePools.find(typeID) != mNodePools.end() && "SceneGraph::AddNodeInternal(uint32, string, UUID, UUID) - Node type not registered before use");

			Node* nodePtr;

			if (id == gInvalidID)
			{
				nodePtr = GetArray(typeID)->AddNodePtr(mEngine, name);

				id = nodePtr->GetID();
			}
			else
			{
				nodePtr = GetArray(typeID)->AddNodePtr(mEngine, name, id);
			}

			AddNodeInternalBase(nodePtr, typeID, id, parentID);

			return nodePtr;
		}

		template<typename T>
		NodePool<T>* GetArray()
		{
			auto type = entt::resolve<T>();
			const auto& typeID = type.id();

			assert(mNodePools.find(typeID) != mNodePools.end() && "SceneGraph::GetArray() - Node type not registered before use");

			return static_cast<NodePool<T>*>(mNodePools.at(typeID));
		}

		INodePool* GetArray(uint32_t typeID)
		{
			assert(mNodePools.find(typeID) != mNodePools.end() && "SceneGraph::GetArray(uint32) - Node type not registered before use");

			return mNodePools.at(typeID);
		}

		std::unordered_map<UUID, uint32_t> mIDToTypeID;
		std::vector<UUID> mNodeIDs; // Vector of node id's, sorted by order methods are executed in
		std::vector<UUID> mRootNodeIDs; // Vector of nodes at root of scene graph

		std::unordered_set<UUID> mNodeTransformsNeedUpdated; // Set of nodes which need their transforms updated
		std::vector<UUID> mNodeTransformsNeedUpdatedVector; // Set of nodes which need their transforms updated
		std::unordered_set<UUID> mNodeTransformsUpToDate; // Set of nodes which global transforms are up to date

		std::unordered_set<UUID> mNodesToDestroy;

		MappedVector<UUID, TransformComponent2D> mGlobalTransform2Ds;
		MappedVector<UUID, TransformComponent3D> mGlobalTransform3Ds;

		bool mSceneGraphUpdated = false;

		std::unordered_map<uint32_t, INodePool*> mNodePools;

	};
}
