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
	template<typename T>
	class NodeFactory
	{
	public:

		NodeFactory() = default;
		~NodeFactory() = default;

		static T Create(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID)
		{
			return T(engine, id);
		}

	};

	class INodeArray
	{
	public:

		INodeArray() = default;

		virtual ~INodeArray() = default;

		virtual Node* AddNodePtr(const std::shared_ptr<core::Engine>& engine, UUID id = gInvalidID) = 0;
		virtual Node* GetNodePtr(UUID id) = 0;
		virtual void RemoveNode(UUID id) = 0;
		virtual bool IsValid(UUID id) = 0;
		virtual void Clear() = 0;

	};

	template<typename T>
	class NodeArray final : public INodeArray
	{
	public:

		NodeArray() = default;
		~NodeArray() override = default;

		T* AddNode(const std::shared_ptr<core::Engine>& engine, UUID id = gInvalidID)
		{
			if (id == gInvalidID)
				id = GenerateId();

			mVector.Emplace(id, mFactory.Create(engine, id));

			return &mVector[id];
		}

		Node* AddNodePtr(const std::shared_ptr<core::Engine>& engine, UUID id = gInvalidID) override
		{
			return static_cast<Node*>(AddNode(engine, id));
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
			mVector.Erase(id);
		}

		bool IsValid(UUID id) override
		{
			return mVector.Contains(id);
		}

		void Clear() override
		{
			mVector.Clear();
		}

	private:

		MappedVector<UUID, T> mVector;
		NodeFactory<T> mFactory;

	};

	class SceneGraphSubsystem : public core::Subsystem
	{
	public:

		explicit SceneGraphSubsystem(const std::shared_ptr<core::Engine>& engine);
		~SceneGraphSubsystem() override = default;

		void Initialize(core::SubsystemManager* subsystemManager) override;

		void EndPlay() override;

		void Update(double deltaTime) override;
		bool ShouldUpdate() override;

		Node* AddNode(uint32_t typeID, UUID id);
		Node* AddChildNode(uint32_t typeID, UUID id, UUID parentID);
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
		void RegisterNodeType()
		{
			auto type = entt::resolve<T>();
			const auto& typeID = type.id();

			if (mNodeArrays.find(typeID) == mNodeArrays.end())
			{
				mNodeArrays.insert({ typeID, static_cast<INodeArray*>(new NodeArray<T>()) });
			}
		}

		template<typename T>
		T* AddNode()
		{
			return AddNodeInternal<T>();
		}

		template<typename T>
		T* AddNode(UUID id)
		{
			return AddNodeInternal<T>(id);
		}

		template<typename T>
		T* AddChildNode(UUID parent_id)
		{
			return AddNodeInternal<T>(gInvalidID, parent_id);
		}

		template<typename T>
		T* AddChildNode(UUID id, UUID parent_id)
		{
			return AddNodeInternal<T>(id, parent_id);
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
		T* AddNodeInternal(UUID id = gInvalidID, UUID parent_id = gInvalidID)
		{
			if (mIDToTypeID.find(id) != mIDToTypeID.end())
			{
				return GetArray<T>()->GetNode(id);
			}

			auto type = entt::resolve<T>();
			const auto& typeID = type.id();

			if (mNodeArrays.find(typeID) == mNodeArrays.end())
			{
				RegisterNodeType<T>();
			}

			assert(mNodeArrays.find(typeID) != mNodeArrays.end() && "SceneGraph::AddNodeInternal(UUID, UUID) - Node type not registered before use");

			Node* nodePtr;

			if (id == gInvalidID)
			{
				T* node = GetArray<T>()->AddNode(mEngine);
				nodePtr = static_cast<Node*>(node);

				id = nodePtr->GetID();
			}
			else
			{
				T* node = GetArray<T>()->AddNode(mEngine, id);
				nodePtr = static_cast<Node*>(node);
			}

			AddNodeInternalBase(nodePtr, typeID, id, parent_id);

			return GetArray<T>()->GetNode(id);
		}

		Node* AddNodeInternal(uint32_t typeID, UUID id = gInvalidID, UUID parentID = gInvalidID)
		{
			if (mIDToTypeID.find(id) != mIDToTypeID.end())
			{
				return GetArray(typeID)->GetNodePtr(id);
			}

			assert(mNodeArrays.find(typeID) != mNodeArrays.end() && "SceneGraph::AddNodeInternal(uint32, UUID, UUID) - Node type not registered before use");

			Node* nodePtr;

			if (id == gInvalidID)
			{
				nodePtr = GetArray(typeID)->AddNodePtr(mEngine);

				id = nodePtr->GetID();
			}
			else
			{
				nodePtr = GetArray(typeID)->AddNodePtr(mEngine, id);
			}

			AddNodeInternalBase(nodePtr, typeID, id, parentID);

			return nodePtr;
		}

		template<typename T>
		NodeArray<T>* GetArray()
		{
			auto type = entt::resolve<T>();
			const auto& typeID = type.id();

			assert(mNodeArrays.find(typeID) != mNodeArrays.end() && "SceneGraph::GetArray() - Node type not registered before use");

			return static_cast<NodeArray<T>*>(mNodeArrays.at(typeID));
		}

		INodeArray* GetArray(uint32_t typeID)
		{
			assert(mNodeArrays.find(typeID) != mNodeArrays.end() && "SceneGraph::GetArray(uint32) - Node type not registered before use");

			return mNodeArrays.at(typeID);
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

		std::unordered_map<uint32_t, INodeArray*> mNodeArrays;

	};
}
