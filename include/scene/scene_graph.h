#pragma once

#include <cassert>
#include <unordered_map>
#include <memory>

#include "node.h"
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

		virtual void create(const PuffinID& id = gInvalidID) = 0;

	protected:

	};

	template<typename T>
	class NodeFactory final : public INodeFactory
	{
	public:

		NodeFactory() {}
		~NodeFactory() override = default;

		T create(const PuffinID& id = gInvalidID) override
		{
			return T(id);
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

		T& add()
		{
			PuffinID id = generateID();

			m_vector.insert(id, m_factory.create(id));

			return &m_vector[id];
		}

		T& add(PuffinID id)
		{
			m_vector.insert(id, m_factory.create(id));

			return m_vector[id];
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
		void register_node()
		{
			const char* type_name = typeid(T).name();

			assert(m_node_arrays.find(type_name) == m_node_arrays.end() && "SceneGraph::register_node() - Registering node type more than once");

			auto array = std::make_unique<NodeArray<T>>();
			m_node_arrays.emplace({ type_name, std::static_pointer_cast<INodeArray>(array) });
		}

		template<typename T>
		T& add_node()
		{
			const char* type_name = typeid(T).name();

			assert(m_node_arrays.find(type_name) != m_node_arrays.end() && "SceneGraph::add_node() - Node type not registered before use");

			return get_array<T>()->add();
		}

		template<typename T>
		T& add_node(PuffinID id)
		{
			const char* type_name = typeid(T).name();

			assert(m_node_arrays.find(type_name) != m_node_arrays.end() && "SceneGraph::add_node(PuffinID) - Node type not registered before use");

			return get_array<T>()->add(id);
		}

		void register_default_nodes();

	private:

		std::unordered_map<std::string, std::unique_ptr<INodeArray>> m_node_arrays;

		template<typename T>
		std::unique_ptr<NodeArray<T>> get_array()
		{
			const char* type_name = typeid(T).name();

			assert(m_node_arrays.find(type_name) != m_node_arrays.end() && "SceneGraph::get_array() - Node type not registered before use");

			return std::static_pointer_cast<NodeArray<T>>(m_node_arrays.at(type_name));
		}

	};
}
