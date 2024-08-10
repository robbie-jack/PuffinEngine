#pragma once

#include <cassert>
#include <unordered_map>
#include <memory>
#include <set>

#include "puffin/core/engine_subsystem.h"
#include "puffin/physics/physics_subsystem.h"
#include "puffin/nodes/node.h"
#include "puffin/types/uuid.h"
#include "puffin/types/packed_array.h"
#include "puffin/types/packed_vector.h"

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

		static T create(const std::shared_ptr<core::Engine>& engine, const PuffinID& id = gInvalidID)
		{
			return T(engine, id);
		}

	};

	class INodeArray
	{
	public:

		INodeArray() = default;

		virtual ~INodeArray() = default;

		virtual Node* add_ptr(const std::shared_ptr<core::Engine>& engine, PuffinID id = gInvalidID) = 0;

		virtual Node* get_ptr(PuffinID id) = 0;

		virtual void remove(PuffinID id) = 0;

		virtual bool valid(PuffinID id) = 0;

		virtual void clear() = 0;

	};

	template<typename T>
	class NodeArray final : public INodeArray
	{
	public:

		NodeArray() = default;
		~NodeArray() override = default;

		T* add(const std::shared_ptr<core::Engine>& engine, PuffinID id = gInvalidID)
		{
			if (id == gInvalidID)
				id = generate_id();

			m_vector.emplace(id, m_factory.create(engine, id));

			return &m_vector[id];
		}

		Node* add_ptr(const std::shared_ptr<core::Engine>& engine, PuffinID id = gInvalidID) override
		{
			return static_cast<Node*>(add(engine, id));
		}

		T* get(PuffinID id)
		{
			return &m_vector.at(id);
		}

		Node* get_ptr(PuffinID id) override
		{
			if (valid(id))
				return static_cast<Node*>(get(id));

			return nullptr;
		}

		void remove(PuffinID id) override
		{
			m_vector.erase(id);
		}

		bool valid(PuffinID id) override
		{
			return m_vector.contains(id);
		}

		void clear() override
		{
			m_vector.clear();
		}

	private:

		PackedVector<PuffinID, T> m_vector;
		NodeFactory<T> m_factory;

	};

	class SceneGraph : public core::EngineSubsystem
	{
	public:

		SceneGraph(const std::shared_ptr<core::Engine>& engine);

		void initialize(core::ISubsystemManager* subsystem_manager) override;

		void end_play() override;

		void engine_update(double delta_time) override;

		template<typename T>
		void register_node_type()
		{
			const char* type_name = typeid(T).name();

			assert(m_node_arrays.find(type_name) == m_node_arrays.end() && "SceneGraph::register_node_type() - Registering node type more than once");

			m_node_arrays.insert({ type_name, static_cast<INodeArray*>(new NodeArray<T>()) });
		}

		template<typename T>
		T* add_node()
		{
			return add_node_internal<T>();
		}

		template<typename T>
		T* add_node(PuffinID id)
		{
			return add_node_internal<T>(id);
		}

		Node* add_node(const char* type_name, PuffinID id);

		template<typename T>
		T* add_child_node(PuffinID parent_id)
		{
			return add_node_internal<T>(gInvalidID, parent_id);
		}

		template<typename T>
		T* add_child_node(PuffinID id, PuffinID parent_id)
		{
			return add_node_internal<T>(id, parent_id);
		}

		Node* add_child_node(const char* type_name, PuffinID id, PuffinID parent_id);

		bool is_valid_node(PuffinID id);

		template<typename T>
		T* get_node(PuffinID id)
		{
			if (!is_valid_node(id))
				return nullptr;

			return get_array<T>()->get(id);
		}

		[[nodiscard]] Node* get_node_ptr(const PuffinID& id);

		[[nodiscard]] const std::string& get_node_type_name(const PuffinID& id) const;

		[[nodiscard]] TransformComponent2D* get_global_transform_2d(const PuffinID& id);

		[[nodiscard]] TransformComponent3D* get_global_transform_3d(const PuffinID& id);

		// Queue a node for destruction, will also destroy all child nodes
		void queue_destroy_node(const PuffinID& id);

		std::vector<PuffinID>& get_node_ids();
		std::vector<PuffinID>& get_root_node_ids();

	private:

		std::unordered_map<PuffinID, std::string> m_id_to_type;
		std::vector<PuffinID> m_node_ids; // Vector of node id's, sorted by order methods are executed in
		std::vector<PuffinID> m_root_node_ids; // Vector of nodes at root of scene graph
		std::set<PuffinID> m_nodes_to_destroy;

		PackedVector<PuffinID, TransformComponent2D> m_global_transform_2ds;
		PackedVector<PuffinID, TransformComponent3D> m_global_transform_3ds;

		bool m_scene_graph_updated = false;

		std::unordered_map<std::string, INodeArray*> m_node_arrays;

		void register_default_node_types();

		void update_scene_graph();
		void destroy_node(PuffinID id);
		void add_id_and_child_ids(PuffinID id, std::vector<PuffinID>& node_ids);

		void update_transforms();
		void apply_local_to_global_transform_2d(PuffinID id, TransformComponent2D& global_transform);
		void apply_local_to_global_transform_3d(PuffinID id, TransformComponent3D& global_transform);

		void add_node_internal_base(Node* node, const char* type_name, PuffinID id = gInvalidID, PuffinID parent_id = gInvalidID)
		{
			if (parent_id != gInvalidID)
			{
				node->set_parent_id(parent_id);

				Node* parent_node_ptr = get_node_ptr(parent_id);
				parent_node_ptr->add_child_id(id);
			}
			else
			{
				m_root_node_ids.push_back(id);
			}

			m_id_to_type.insert({ id, type_name });

			if (node->has_transform_2d())
			{
				m_global_transform_2ds.emplace(id, TransformComponent2D());
			}

			if (node->has_transform_3d())
			{
				m_global_transform_3ds.emplace(id, TransformComponent3D());
			}

			m_scene_graph_updated = true;
		}

		template<typename T>
		T* add_node_internal(PuffinID id = gInvalidID, PuffinID parent_id = gInvalidID)
		{
			if (m_id_to_type.find(id) != m_id_to_type.end())
			{
				return get_array<T>()->get(id);
			}

			const char* type_name = typeid(T).name();

			if (m_node_arrays.find(type_name) == m_node_arrays.end())
			{
				register_node_type<T>();
			}

			assert(m_node_arrays.find(type_name) != m_node_arrays.end() && "SceneGraph::add_node_internal(PuffinID, PuffinID) - Node type not registered before use");

			Node* node_ptr;

			if (id == gInvalidID)
			{
				T* node = get_array<T>()->add(m_engine);
				node_ptr = static_cast<Node*>(node);

				id = node_ptr->id();
			}
			else
			{
				T* node = get_array<T>()->add(m_engine, id);
				node_ptr = static_cast<Node*>(node);
			}

			add_node_internal_base(node_ptr, type_name, id, parent_id);

			return get_array<T>()->get(id);
		}

		Node* add_node_internal(const char* type_name, PuffinID id = gInvalidID, PuffinID parent_id = gInvalidID)
		{
			if (m_id_to_type.find(id) != m_id_to_type.end())
			{
				return get_array(type_name)->get_ptr(id);
			}

			assert(m_node_arrays.find(type_name) != m_node_arrays.end() && "SceneGraph::add_node_internal(const char*, PuffinID, PuffinID) - Node type not registered before use");

			Node* node_ptr;

			if (id == gInvalidID)
			{
				node_ptr = get_array(type_name)->add_ptr(m_engine);

				id = node_ptr->id();
			}
			else
			{
				node_ptr = get_array(type_name)->add_ptr(m_engine, id);
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

	class SceneGraphGameplay : public physics::PhysicsSubsystem
	{
	public:

		explicit SceneGraphGameplay(const std::shared_ptr<core::Engine>& engine);

		void initialize(core::ISubsystemManager* subsystem_manager) override;

		void update(double delta_time) override;
		void fixed_update(double fixed_time) override;

	};
}
