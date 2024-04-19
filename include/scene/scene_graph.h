#pragma once

#include <cassert>
#include <unordered_map>
#include <memory>
#include <set>

#include "scene/node.h"
#include "Core/System.h"
#include "Types/UUID.h"
#include "Types/PackedArray.h"

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

		INodeFactory() {}

		virtual ~INodeFactory() = default;

	protected:

	};

	template<typename T>
	class NodeFactory final : public INodeFactory
	{
	public:

		NodeFactory() {}
		~NodeFactory() override = default;

		static T create(const std::shared_ptr<core::Engine>& engine, const PuffinID& id = gInvalidID)
		{
			return T(engine, id);
		}

	private:



	};

	class INodeArray
	{
	public:

		INodeArray() = default;

		virtual ~INodeArray() = default;

	protected:



	};

	template<typename T>
	class NodeArray final : public INodeArray
	{
	public:

		NodeArray() = default;
		~NodeArray() override = default;

		T& add(const std::shared_ptr<core::Engine>& engine)
		{
			PuffinID id = generateID();

			m_vector.insert(id, m_factory.create(engine, id));

			return &m_vector[id];
		}

		T& add(const std::shared_ptr<core::Engine>& engine, PuffinID id)
		{
			m_vector.insert(id, m_factory.create(engine, id));

			return m_vector[id];
		}

		T& get(PuffinID id)
		{
			return m_vector.at(id);
		}

		void remove(PuffinID id)
		{
			m_vector.erase(id);
		}

		bool valid(PuffinID id)
		{
			return m_vector.contains(id);
		}

	private:

		PackedVector<T> m_vector;
		NodeFactory<T> m_factory;

	};

	class SceneGraph : public core::System
	{
	public:

		SceneGraph(const std::shared_ptr<core::Engine>& engine);

		template<typename T>
		void register_node_type()
		{
			const char* type_name = typeid(T).name();

			assert(m_node_arrays.find(type_name) == m_node_arrays.end() && "SceneGraph::register_node_type() - Registering node type more than once");

			m_node_arrays.insert({ type_name, static_cast<INodeArray*>(new NodeArray<T>()) });
		}

		template<typename T>
		T& add_node()
		{
			const char* type_name = typeid(T).name();

			assert(m_node_arrays.find(type_name) != m_node_arrays.end() && "SceneGraph::add_node() - Node type not registered before use");

			T& node = get_array<T>()->add(mEngine);

			Node* node_ptr = static_cast<Node*>(*node);

			m_id_to_nodes.insert({ node_ptr->id(), node_ptr });
			m_nodes_unsorted.push_back(node_ptr);

			m_scene_graph_updated = true;

			return node;
		}

		template<typename T>
		T& add_node(PuffinID id)
		{
			const char* type_name = typeid(T).name();

			assert(m_node_arrays.find(type_name) != m_node_arrays.end() && "SceneGraph::add_node(PuffinID) - Node type not registered before use");

			T& node = get_array<T>()->add(mEngine, id);

			Node* node_ptr = static_cast<Node*>(*node);

			m_id_to_nodes.insert({ node_ptr->id(), node_ptr });
			m_nodes_unsorted.push_back(node_ptr);

			m_scene_graph_updated = true;

			return node;
		}

		[[nodiscard]] Node* get_node(const PuffinID& id) const
		{
			if (m_id_to_nodes.count(id) != 0)
				return m_id_to_nodes.at(id);

			return nullptr;
		}

		// Queue a node for destruction, will also destroy all child nodes
		void queue_destroy_node(const PuffinID& id)
		{
			m_nodes_to_destroy.insert(id);
		}

		void register_default_nodes();
		void begin_play();
		void update();
		void physics_update();
		void end_play();

	private:

		std::unordered_map<PuffinID, Node*> m_id_to_nodes;
		std::vector<Node*> m_nodes_unsorted;
		std::vector<Node*> m_nodes_sorted;
		std::set<PuffinID> m_nodes_to_destroy;

		bool m_scene_graph_updated = false;

		std::unordered_map<std::string, INodeArray*> m_node_arrays;

		template<typename T>
		NodeArray<T>* get_array()
		{
			const char* type_name = typeid(T).name();

			assert(m_node_arrays.find(type_name) != m_node_arrays.end() && "SceneGraph::get_array() - Node type not registered before use");

			return m_node_arrays.at(type_name);
		}

	};
}
