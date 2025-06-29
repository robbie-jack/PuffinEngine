#pragma once

#include <cassert>
#include <unordered_map>
#include <memory>
#include <unordered_set>

#include "core/subsystem.h"
#include "node/node.h"
#include "types/uuid.h"
#include "types/storage/mapped_array.h"
#include "types/storage/mapped_vector.h"

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
	constexpr uint32_t gDefaultNodePoolSize = 128;

	// PFN_TODO_SERIALIZATION - Rework node serialization logic to match component implementation

	class INodePool
	{
	public:

		INodePool() = default;

		virtual ~INodePool() = default;

		virtual Node* AddNode(const std::shared_ptr<core::Engine>& engine, const std::string& name, UUID id = gInvalidID) = 0;
		virtual Node* GetNode(UUID id) = 0;
		virtual void RemoveNode(UUID id) = 0;
		virtual bool IsValid(UUID id) = 0;
		virtual void Resize(uint32_t newSize, bool forceShrink = false) = 0;
		virtual void Reset() = 0;
		virtual void Clear() = 0;

	};

	template<typename T>
	class NodePool final : public INodePool
	{
	public:

		NodePool() = default;

		~NodePool() override = default;

		Node* AddNode(const std::shared_ptr<core::Engine>& engine, const std::string& name, UUID id = gInvalidID) override
		{
			if (id == gInvalidID)
				id = GenerateId();

			if (mNodes.Full())
			{
				uint32_t newSize = mNodes.Size() * 2;

				mNodes.Resize(newSize);
			}

			mNodes.Emplace(id, T{});

			T& node = mNodes.At(id);
			auto* nodePtr = static_cast<Node*>(&node);
			nodePtr->Prepare(engine, name, id);

			return &node;
		}

		Node* GetNode(UUID id) override
		{
			if (IsValid(id))
				return static_cast<Node*>(GetNodeTyped(id));

			return nullptr;
		}

		void RemoveNode(UUID id) override
		{
			GetNode(id)->Reset();

			mNodes.Erase(id, false);
		}

		bool IsValid(UUID id) override
		{
			return mNodes.Contains(id);
		}

		void Resize(uint32_t newSize, bool forceShrink = false) override
		{
			if (newSize > mNodes.Size() || (newSize < mNodes.Size() && forceShrink))
				mNodes.Resize(newSize);
		}

		/*
		 * Reset all nodes in pool
		 */
		void Reset() override
		{
			for (auto& node : mNodes)
			{
				node.Reset();
			}

			mNodes.Clear(false);
		}

		/*
		 * Reset & clear all nodes in pool
		 */
		void Clear() override
		{
			for (auto& node : mNodes)
			{
				node.Reset();
			}

			mNodes.Clear();
		}

		T* GetNodeTyped(UUID id)
		{
			return &mNodes.At(id);
		}

		void GetNodes(std::vector<T*>& nodes)
		{
			nodes.resize(mNodes.Count());

			int i = 0;
			for (auto& node : mNodes)
			{
				nodes[i] = &node;

				i++;
			}
		}

	private:

		MappedVector<UUID, T> mNodes;

	};

	class SceneGraphSubsystem : public core::Subsystem
	{
	public:

		explicit SceneGraphSubsystem(const std::shared_ptr<core::Engine>& engine);
		~SceneGraphSubsystem() override = default;

		void RegisterTypes() override;

		void PreInitialize() override;
		void Initialize(core::SubsystemManager* subsystemManager) override;
		void Deinitialize() override;

		void EndPlay() override;

		void Update(double deltaTime) override;
		bool ShouldUpdate() override;

		Node* AddNode(uint32_t typeID, const std::string& name, UUID id);
		Node* AddChildNode(uint32_t typeID, const std::string& name, UUID id, UUID parentID);
		[[nodiscard]] Node* GetNode(const UUID& id) const;
		bool IsValidNode(UUID id) const;

		// PUFFIN_TODO - Remove when refactoring 3d nodes to remove reliance on components
		[[nodiscard]] const TransformComponent3D& GetNodeGlobalTransform3D(const UUID& id) const;
		[[nodiscard]] TransformComponent3D& GetNodeGlobalTransform3D(const UUID& id);

		// PUFFIN_TODO - Remove when refactoring 3d nodes to remove reliance on components
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

			if (mNodePools.find(typeID) == mNodePools.end())
			{
				mNodePools.emplace(typeID, static_cast<INodePool*>(new NodePool<T>()));
			}
		}

		template<typename T>
		void SetDefaultNodePoolSize(uint32_t defaultPoolSize)
		{
			auto type = entt::resolve<T>();
			const auto& typeID = type.id();

			if (mNodePoolDefaultSizes.find(typeID) == mNodePoolDefaultSizes.end())
			{
				mNodePoolDefaultSizes.emplace(typeID, defaultPoolSize);
			}
			else
			{
				mNodePoolDefaultSizes[typeID] = defaultPoolSize;
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
		T* GetNode(UUID id) const
		{
			if (!IsValidNode(id))
				return nullptr;

			return static_cast<T*>(GetPool<T>()->GetNode(id));
		}

		template<typename T>
		void GetNodes(std::vector<T*>& nodes) const
		{
			auto type = entt::resolve<T>();
			const auto& typeID = type.id();

			// PUFFIN_TODO - Add output here to show that no node of that type was registered
			if (mNodePools.find(typeID) == mNodePools.end())
				return;

			GetPool<T>()->GetNodes(nodes);
		}

	private:

		void UpdateSceneGraph();
		void DestroyNode(UUID id);
		void AddIDAndChildIDs(UUID id, std::vector<UUID>& nodeIDs);

		void UpdateGlobalTransforms();
		void UpdateGlobalTransform(UUID id);

		static void LimitAngleTo180Degrees(float& angle);
		static void ApplyLocalToGlobalTransform3D(const TransformComponent3D& localTransform, const TransformComponent3D& globalTransform, TransformComponent3D&
		                                          updatedTransform);

		void AddNodeInternalBase(Node* node, uint32_t typeID, UUID id = gInvalidID, UUID parentID = gInvalidID);

		template<typename T>
		T* AddNodeInternal(const std::string& name, UUID id = gInvalidID, UUID parent_id = gInvalidID)
		{
			if (mIDToTypeID.find(id) != mIDToTypeID.end())
			{
				return GetPool<T>()->GetNodeTyped(id);
			}

			auto type = entt::resolve<T>();
			const auto& typeID = type.id();

			if (mNodePools.find(typeID) == mNodePools.end())
			{
				RegisterNodeType<T>();
			}

			Node* node = nullptr;

			if (id == gInvalidID)
			{
				node = GetPool<T>()->AddNode(mEngine, name);
				id = node->GetID();
			}
			else
			{
				node = GetPool<T>()->AddNode(mEngine, name, id);
			}

			AddNodeInternalBase(node, typeID, id, parent_id);

			return GetPool<T>()->GetNodeTyped(id);
		}

		Node* AddNodeInternal(uint32_t typeID, const std::string& name, UUID id = gInvalidID, UUID parentID = gInvalidID)
		{
			if (mIDToTypeID.find(id) != mIDToTypeID.end())
			{
				return GetPool(typeID)->GetNode(id);
			}

			assert(mNodePools.find(typeID) != mNodePools.end() && "SceneGraph::AddNodeInternal(uint32, string, UUID, UUID) - Node type not registered before use");

			Node* node;

			if (id == gInvalidID)
			{
				node = GetPool(typeID)->AddNode(mEngine, name);

				id = node->GetID();
			}
			else
			{
				node = GetPool(typeID)->AddNode(mEngine, name, id);
			}

			AddNodeInternalBase(node, typeID, id, parentID);

			return node;
		}

		template<typename T>
		NodePool<T>* GetPool() const
		{
			auto type = entt::resolve<T>();
			const auto& typeID = type.id();

			assert(mNodePools.find(typeID) != mNodePools.end() && "SceneGraph::GetPool() - Node type not registered before use");

			return static_cast<NodePool<T>*>(mNodePools.at(typeID));
		}

		INodePool* GetPool(uint32_t typeID) const
		{
			assert(mNodePools.find(typeID) != mNodePools.end() && "SceneGraph::GetPool(uint32) - Node type not registered before use");

			return mNodePools.at(typeID);
		}

		std::unordered_map<UUID, uint32_t> mIDToTypeID;
		std::vector<UUID> mNodeIDs; // Vector of node id's, sorted by order methods are executed in
		std::vector<UUID> mRootNodeIDs; // Vector of nodes at root of scene graph

		std::unordered_set<UUID> mNodeTransformsNeedUpdated; // Set of nodes which need their transforms updated
		std::vector<UUID> mNodeTransformsNeedUpdatedVector; // Set of nodes which need their transforms updated
		std::unordered_set<UUID> mNodeTransformsUpToDate; // Set of nodes which global transforms are up to date

		std::unordered_set<UUID> mNodesToDestroy;

		MappedVector<UUID, TransformComponent3D> mGlobalTransform3Ds;

		bool mSceneGraphUpdated = false;

		std::unordered_map<uint32_t, INodePool*> mNodePools;
		std::unordered_map<uint32_t, uint32_t> mNodePoolDefaultSizes;

	};
}
