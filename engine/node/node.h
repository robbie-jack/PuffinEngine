#pragma once

#include <list>
#include <memory>
#include <entt/entity/registry.hpp>

#include "types/uuid.h"
#include "utility/reflection.h"
#include "utility/serialization.h"

namespace puffin
{
	namespace scene
	{
		class SceneGraphSubsystem;
	}

	namespace core
	{
		class Engine;
	}

	namespace ecs
	{
		class EnTTSubsystem;
	}

	const std::string gNodeTypeString = "Node";

	// PFN_TODO_SERIALIZATION - Remove as part of node serialization rework
	struct NodeCustomData
	{
		explicit NodeCustomData(const std::string& nodeTypeString) : nodeTypeString(nodeTypeString) {}

		std::string nodeTypeString;
	};

	class Node
	{
	public:

		explicit Node() = default;
		virtual ~Node() = default;

		/*
		 * Called by node pool when a new node is requested, sets engine, registry, id, name (optional)
		 */
		void Prepare(const std::shared_ptr<core::Engine>& engine, const std::string& name, UUID id);

		/*
		 * Called by node pool when node is destroyed to free it up for reuse
		 */
		void Reset();

		/*
		 * Initialize node, called upon being added to scene
		 * Add any components you want here
		 */
		virtual void Initialize();

		/*
		 * Deinitialize node, called upon being removed from scene
		 * Remove any added components here
		 */
		virtual void Deinitialize();

		virtual void BeginPlay();
		virtual void EndPlay();

		virtual void Update(double deltaTime);
		[[nodiscard]] virtual bool ShouldUpdate() const;

		virtual void FixedUpdate(double deltaTime);
		[[nodiscard]] virtual bool ShouldFixedUpdate() const;

		virtual void Serialize(nlohmann::json& json) const;
		virtual void Deserialize(const nlohmann::json& json);

		/*
		 * Return type string associated with this node type
		 * This should be be overriden by inheriting types
		 */
		[[nodiscard]] virtual const std::string& GetTypeString() const;

		/*
		 * Return type id associated with this node type
		 * This should be be overriden by inheriting types
		 */
		[[nodiscard]] virtual entt::id_type GetTypeID() const;

		[[nodiscard]] UUID GetID() const;
		[[nodiscard]] entt::entity GetEntity() const;

		[[nodiscard]] const std::string& GetName() const;
		void SetName(const std::string& name);

		/*
		 * Queues node to be destroyed, will also destroy all child nodes
		 */
		void QueueDestroy() const;

		[[nodiscard]] Node* GetParent() const;
		[[nodiscard]] Node* GetChild(UUID id) const;
		void Reparent(const UUID& id);
		void GetChildren(std::vector<Node*>& children) const;
		[[nodiscard]] const std::list<UUID>& GetChildIDs() const;
		[[nodiscard]] bool HasChildren() const;

		void RemoveChild(UUID id);

		[[nodiscard]] UUID GetParentID() const;

		// Set parent id, for internal use only, use reparent instead
		void SetParentID(UUID id);

		// Add a child id, for internal use only, use add_child instead
		void AddChildID(UUID id);

		// Remove a child id, for internal use only, use remove_child instead
		void RemoveChildID(UUID id);

		template<typename T>
		T& GetComponent()
		{
			return mRegistry->get<T>(mEntity);
		}

		template<typename T>
		const T& GetComponent() const
		{
			return mRegistry->get<T>(mEntity);
		}

		template<typename T>
		[[nodiscard]] bool HasComponent() const
		{
			return mRegistry->any_of<T>(mEntity);
		}

		template<typename T>
		T& AddComponent()
		{
			return mRegistry->get_or_emplace<T>(mEntity);
		}

		template<typename T>
		void RemoveComponent() const
		{
			mRegistry->remove<T>(mEntity);
		}

	protected:

		UUID mNodeID = gInvalidID;
		std::string mName;

		entt::entity mEntity;

		UUID mParentID = gInvalidID;
		std::list<UUID> mChildIDs;

		std::shared_ptr<core::Engine> mEngine = nullptr;
		std::shared_ptr<entt::registry> mRegistry = nullptr;
	};

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<Node>()
		{
			return "Node";
		}

		template<>
		inline entt::hs GetTypeHashedString<Node>()
		{
			return entt::hs(GetTypeString<Node>().data());
		}

		template<>
		inline void RegisterType<Node>()
		{
			auto meta = entt::meta<Node>();

			reflection::RegisterTypeDefaults(meta);
		}
	}
}
