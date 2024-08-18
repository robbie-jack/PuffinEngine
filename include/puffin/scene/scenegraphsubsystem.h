#pragma once

#include <cassert>
#include <unordered_map>
#include <memory>
#include <set>

#include "puffin/core/subsystem.h"
#include "puffin/nodes/node.h"
#include "puffin/types/uuid.h"
#include "puffin/types/packedarray.h"
#include "puffin/types/packedvector.h"

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

			mVector.emplace(id, mFactory.Create(engine, id));

			return &mVector[id];
		}

		Node* AddNodePtr(const std::shared_ptr<core::Engine>& engine, UUID id = gInvalidID) override
		{
			return static_cast<Node*>(AddNode(engine, id));
		}

		T* GetNode(UUID id)
		{
			return &mVector.at(id);
		}

		Node* GetNodePtr(UUID id) override
		{
			if (IsValid(id))
				return static_cast<Node*>(GetNode(id));

			return nullptr;
		}

		void RemoveNode(UUID id) override
		{
			mVector.erase(id);
		}

		bool IsValid(UUID id) override
		{
			return mVector.contains(id);
		}

		void Clear() override
		{
			mVector.clear();
		}

	private:

		PackedVector<UUID, T> mVector;
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

		Node* AddNode(const char* typeName, UUID id);
		Node* AddChildNode(const char* typeName, UUID id, UUID parentID);
		[[nodiscard]] Node* GetNode(const UUID& id);
		bool IsValidNode(UUID id);

		[[nodiscard]] const std::string& GetNodeTypeName(const UUID& id) const;

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
			const char* typeName = typeid(T).name();

			assert(mNodeArrays.find(typeName) == mNodeArrays.end() && "SceneGraph::RegisterNodeType() - Registering node type more than once");

			mNodeArrays.insert({ typeName, static_cast<INodeArray*>(new NodeArray<T>()) });
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
		static void ApplyLocalToGlobalTransform2D(const TransformComponent2D& localTransform, TransformComponent2D& globalTransform);
		static void ApplyLocalToGlobalTransform3D(const TransformComponent3D& localTransform, TransformComponent3D& globalTransform);

		void AddNodeInternalBase(Node* node, const char* typeName, UUID id = gInvalidID, UUID parentID = gInvalidID);

		template<typename T>
		T* AddNodeInternal(UUID id = gInvalidID, UUID parent_id = gInvalidID)
		{
			if (mIDToType.find(id) != mIDToType.end())
			{
				return GetArray<T>()->GetNode(id);
			}

			const char* typeName = typeid(T).name();

			if (mNodeArrays.find(typeName) == mNodeArrays.end())
			{
				RegisterNodeType<T>();
			}

			assert(mNodeArrays.find(typeName) != mNodeArrays.end() && "SceneGraph::AddNodeInternal(PuffinID, PuffinID) - Node type not registered before use");

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

			AddNodeInternalBase(nodePtr, typeName, id, parent_id);

			return GetArray<T>()->GetNode(id);
		}

		Node* AddNodeInternal(const char* type_name, UUID id = gInvalidID, UUID parent_id = gInvalidID)
		{
			if (mIDToType.find(id) != mIDToType.end())
			{
				return GetArray(type_name)->GetNodePtr(id);
			}

			assert(mNodeArrays.find(type_name) != mNodeArrays.end() && "SceneGraph::AddNodeInternal(const char*, PuffinID, PuffinID) - Node type not registered before use");

			Node* nodePtr;

			if (id == gInvalidID)
			{
				nodePtr = GetArray(type_name)->AddNodePtr(mEngine);

				id = nodePtr->GetID();
			}
			else
			{
				nodePtr = GetArray(type_name)->AddNodePtr(mEngine, id);
			}

			AddNodeInternalBase(nodePtr, type_name, id, parent_id);

			return nodePtr;
		}

		template<typename T>
		NodeArray<T>* GetArray()
		{
			const char* typeName = typeid(T).name();

			assert(mNodeArrays.find(typeName) != mNodeArrays.end() && "SceneGraph::GetArray() - Node type not registered before use");

			return static_cast<NodeArray<T>*>(mNodeArrays.at(typeName));
		}

		INodeArray* GetArray(const char* type_name)
		{
			assert(mNodeArrays.find(type_name) != mNodeArrays.end() && "SceneGraph::GetArray(const char*) - Node type not registered before use");

			return mNodeArrays.at(type_name);
		}

		std::unordered_map<UUID, std::string> mIDToType;
		std::vector<UUID> mNodeIDs; // Vector of node id's, sorted by order methods are executed in
		std::vector<UUID> mRootNodeIDs; // Vector of nodes at root of scene graph
		std::set<UUID> mNodeTransformsToUpdate; // Set of nodes which need their transforms updated
		std::set<UUID> mNodeTransformsAlreadyUpdated; // Set of nodes which have already had their transforms updated this frame
		std::set<UUID> mNodesToDestroy;

		PackedVector<UUID, TransformComponent2D> mGlobalTransform2Ds;
		PackedVector<UUID, TransformComponent3D> mGlobalTransform3Ds;

		bool mSceneGraphUpdated = false;

		std::unordered_map<std::string, INodeArray*> mNodeArrays;

	};
}
