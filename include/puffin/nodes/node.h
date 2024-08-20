#pragma once

#include <list>
#include <memory>
#include <entt/entity/registry.hpp>

#include "puffin/types/uuid.h"
#include "puffin/utility/serialization.h"

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

	class Node
	{
	public:

		explicit Node(const std::shared_ptr<core::Engine>& engine, const UUID& id = gInvalidID);
		virtual ~Node() = default;

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

		virtual void Serialize(serialization::Archive& archive) const;
		virtual void Deserialize(const serialization::Archive& archive);

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
		void GetChildIDs(std::vector<UUID>& childIDs) const;

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
		entt::entity mEntity;

		UUID mParentID = gInvalidID;
		std::list<UUID> mChildIDs;
		std::string mName;

		std::shared_ptr<core::Engine> mEngine = nullptr;
		std::shared_ptr<entt::registry> mRegistry = nullptr;
	};
}
