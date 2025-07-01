#pragma once

#include "core/engine.h"
#include "entt/entity/registry.hpp"
#include "types/vector2.h"
#include "subsystem/engine_subsystem.h"

namespace puffin
{
	namespace rendering
	{
		struct ProceduralMeshComponent3D;
	}
}

namespace puffin
{
	namespace procedural
	{
		struct ProceduralIcoSphereComponent3D;
		struct ProceduralTerrainComponent3D;

		class ProceduralMeshGenSystem : public core::EngineSubsystem
		{
		public:

			explicit ProceduralMeshGenSystem(const std::shared_ptr<core::Engine>& engine);
			~ProceduralMeshGenSystem() override;

			std::string_view GetName() const override;

			static void OnConstructPlane(entt::registry& registry, entt::entity entity);
			static void OnConstructTerrain(entt::registry& registry, entt::entity entity);
			static void OnConstructIcoSphere(entt::registry& registry, entt::entity entity);

		private:

			// Generator list of vertices/indices for a flat plane
			static void GeneratePlaneVertices(const Vector2f& half_size, const Vector2i& num_quads, rendering::ProceduralMeshComponent3D& mesh);
			static void GenerateTerrain(const ProceduralTerrainComponent3D& terrain, rendering::ProceduralMeshComponent3D& mesh);
			static void GenerateIcoSphere(const ProceduralIcoSphereComponent3D& sphere, rendering::ProceduralMeshComponent3D& mesh);

		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<procedural::ProceduralMeshGenSystem>()
		{
			return "ProceduralMeshGenSystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<procedural::ProceduralMeshGenSystem>()
		{
			return entt::hs(GetTypeString<procedural::ProceduralMeshGenSystem>().data());
		}

		template<>
		inline void RegisterType<procedural::ProceduralMeshGenSystem>()
		{
			auto meta = entt::meta<procedural::ProceduralMeshGenSystem>()
				.base<core::EngineSubsystem>()
				.base<core::Subsystem>();

			RegisterTypeDefaults(meta);
			RegisterSubsystemDefault(meta);
		}
	}
}
