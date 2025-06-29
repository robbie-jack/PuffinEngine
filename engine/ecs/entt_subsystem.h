#pragma once

#include <unordered_map>
#include <unordered_set>

#include "subsystem/engine_subsystem.h"
#include "types/uuid.h"
#include "core/engine.h"
#include "entt/entity/registry.hpp"

namespace puffin
{
	namespace ecs
	{
		class EnTTSubsystem : public core::EngineSubsystem
		{
		public:

			explicit EnTTSubsystem(const std::shared_ptr<core::Engine>& engine);
			~EnTTSubsystem() override;

			void Initialize() override;
			void Deinitialize() override;

			void EndPlay() override;

			std::string_view GetName() const override;

			UUID AddEntity(bool shouldBeSerialized = true);

			// Add an entity using an existing id
			entt::entity AddEntity(UUID id, bool shouldBeSerialized = true);

			void RemoveEntity(UUID id);

			bool IsEntityValid(UUID id) const;

			[[nodiscard]] entt::entity GetEntity(UUID id) const;
			[[nodiscard]] UUID GetID(entt::entity entity) const;

			[[nodiscard]] bool ShouldEntityBeSerialized(const UUID& id) const;

			std::shared_ptr<entt::registry> GetRegistry();

		private:

			std::shared_ptr<entt::registry> m_registry = nullptr;

			std::unordered_map<UUID, entt::entity> m_idToEntity;
			std::unordered_map<entt::entity, UUID> m_entityToId;
			std::unordered_set<UUID> m_shouldBeSerialized;

		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<ecs::EnTTSubsystem>()
		{
			return "EnTTSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<ecs::EnTTSubsystem>()
		{
			return entt::hs(GetTypeString<ecs::EnTTSubsystem>().data());
		}

		template<>
		inline void RegisterType<ecs::EnTTSubsystem>()
		{
			auto meta = entt::meta<ecs::EnTTSubsystem>()
				.base<core::EngineSubsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}
