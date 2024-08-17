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
	class INodeFactory
	{
	public:

		INodeFactory() = default;

		virtual ~INodeFactory() = default;

	protected:

	};

	template<typename T>
	class NodeFactory final : public INodeFactory
	{
	public:

		NodeFactory() = default;
		~NodeFactory() override = default;

		static T create(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID)
		{
			return T(engine, id);
		}

	};

	class INodeArray
	{
	public:

		INodeArray() = default;

		virtual ~INodeArray() = default;

		virtual Node* add_ptr(const std::shared_ptr<core::Engine>& engine, UUID id = gInvalidID) = 0;

		virtual Node* get_ptr(UUID id) = 0;

		virtual void remove(UUID id) = 0;

		virtual bool valid(UUID id) = 0;

		virtual void clear() = 0;

	};

	template<typename T>
	class NodeArray final : public INodeArray
	{
	public:

		NodeArray() = default;
		~NodeArray() override = default;

		T* add(const std::shared_ptr<core::Engine>& engine, UUID id = gInvalidID)
		{
			if (id == gInvalidID)
				id = GenerateId();

			m_vector.emplace(id, m_factory.create(engine, id));

			return &m_vector[id];
		}

		Node* add_ptr(const std::shared_ptr<core::Engine>& engine, UUID id = gInvalidID) override
		{
			return static_cast<Node*>(add(engine, id));
		}

		T* get(UUID id)
		{
			return &m_vector.at(id);
		}

		Node* get_ptr(UUID id) override
		{
			if (valid(id))
				return static_cast<Node*>(get(id));

			return nullptr;
		}

		void remove(UUID id) override
		{
			m_vector.erase(id);
		}

		bool valid(UUID id) override
		{
			return m_vector.contains(id);
		}

		void clear() override
		{
			m_vector.clear();
		}

	private:

		PackedVector<UUID, T> m_vector;
		NodeFactory<T> m_factory;

	};

	class SceneGraphSubsystem : public core::Subsystem
	{
	public:

		explicit SceneGraphSubsystem(const std::shared_ptr<core::Engine>& engine);
		~SceneGraphSubsystem() = default;

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

			assert(m_node_arrays.find(typeName) == m_node_arrays.end() && "SceneGraph::RegisterNodeType() - Registering node type more than once");

			m_node_arrays.insert({ typeName, static_cast<INodeArray*>(new NodeArray<T>()) });
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

			return get_array<T>()->get(id);
		}

	private:

		std::unordered_map<UUID, std::string> m_id_to_type;
		std::vector<UUID> m_node_ids; // Vector of node id's, sorted by order methods are executed in
		std::vector<UUID> m_root_node_ids; // Vector of nodes at root of scene graph
		std::set<UUID> mNodeTransformsToUpdate;
		std::set<UUID> m_nodes_to_destroy;

		PackedVector<UUID, TransformComponent2D> m_global_transform_2ds;
		PackedVector<UUID, TransformComponent3D> m_global_transform_3ds;

		bool m_scene_graph_updated = false;

		std::unordered_map<std::string, INodeArray*> m_node_arrays;

		void RegisterDefaultNodeTypes();

		void update_scene_graph();
		void destroy_node(UUID id);
		void add_id_and_child_ids(UUID id, std::vector<UUID>& node_ids);

		void update_transforms();
		void apply_local_to_global_transform_2d(UUID id, TransformComponent2D& global_transform);
		void apply_local_to_global_transform_3d(UUID id, TransformComponent3D& global_transform);

		void add_node_internal_base(Node* node, const char* type_name, UUID id = gInvalidID, UUID parent_id = gInvalidID)
		{
			if (parent_id != gInvalidID)
			{
				node->SetParentID(parent_id);

				Node* parent_node_ptr = GetNode(parent_id);
				parent_node_ptr->AddChildID(id);
			}
			else
			{
				m_root_node_ids.push_back(id);
			}

			m_id_to_type.insert({ id, type_name });

			if (node->HasTransform2D())
			{
				m_global_transform_2ds.emplace(id, TransformComponent2D());
			}

			if (node->HasTransform3D())
			{
				m_global_transform_3ds.emplace(id, TransformComponent3D());
			}

			m_scene_graph_updated = true;
		}

		template<typename T>
		T* AddNodeInternal(UUID id = gInvalidID, UUID parent_id = gInvalidID)
		{
			if (m_id_to_type.find(id) != m_id_to_type.end())
			{
				return get_array<T>()->get(id);
			}

			const char* type_name = typeid(T).name();

			if (m_node_arrays.find(type_name) == m_node_arrays.end())
			{
				RegisterNodeType<T>();
			}

			assert(m_node_arrays.find(type_name) != m_node_arrays.end() && "SceneGraph::add_node_internal(PuffinID, PuffinID) - Node type not registered before use");

			Node* node_ptr;

			if (id == gInvalidID)
			{
				T* node = get_array<T>()->add(mEngine);
				node_ptr = static_cast<Node*>(node);

				id = node_ptr->GetID();
			}
			else
			{
				T* node = get_array<T>()->add(mEngine, id);
				node_ptr = static_cast<Node*>(node);
			}

			add_node_internal_base(node_ptr, type_name, id, parent_id);

			return get_array<T>()->get(id);
		}

		Node* AddNodeInternal(const char* type_name, UUID id = gInvalidID, UUID parent_id = gInvalidID)
		{
			if (m_id_to_type.find(id) != m_id_to_type.end())
			{
				return get_array(type_name)->get_ptr(id);
			}

			assert(m_node_arrays.find(type_name) != m_node_arrays.end() && "SceneGraph::add_node_internal(const char*, PuffinID, PuffinID) - Node type not registered before use");

			Node* node_ptr;

			if (id == gInvalidID)
			{
				node_ptr = get_array(type_name)->add_ptr(mEngine);

				id = node_ptr->GetID();
			}
			else
			{
				node_ptr = get_array(type_name)->add_ptr(mEngine, id);
			}

			add_node_internal_base(node_ptr, type_name, id, parent_id);

			return node_ptr;
		}

		template<typename T>
		NodeArray<T>* get_array()
		{
			const char* type_name = typeid(T).name();

			assert(m_node_arrays.find(type_name) != m_node_arrays.end() && "SceneGraph::get_array() - Node type not registered before use");

			return static_cast<NodeArray<T>*>(m_node_arrays.at(type_name));
		}

		INodeArray* get_array(const char* type_name)
		{
			assert(m_node_arrays.find(type_name) != m_node_arrays.end() && "SceneGraph::get_array(const char*) - Node type not registered before use");

			return m_node_arrays.at(type_name);
		}

	};

	class SceneGraphGameplaySubsystem : public core::Subsystem
	{
	public:

		explicit SceneGraphGameplaySubsystem(const std::shared_ptr<core::Engine>& engine);
		~SceneGraphGameplaySubsystem() override = default;

		void Initialize(core::SubsystemManager* subsystem_manager) override;

		[[nodiscard]] core::SubsystemType GetType() const override;

		void Update(double delta_time) override;
		bool ShouldUpdate() override;

		void FixedUpdate(double fixed_time) override;
		bool ShouldFixedUpdate() override;

	};
}
