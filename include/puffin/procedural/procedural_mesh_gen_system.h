#pragma once

#include "puffin/core/engine.h"
#include "entt/entity/registry.hpp"
#include "puffin/types/vector.h"
#include "puffin/core/system.h"
#include "puffin/ecs/entt_subsystem.h"

namespace puffin
{
	namespace rendering
	{
		struct ProceduralMeshComponent;
	}
}

namespace puffin::procedural
{
	struct IcoSphereComponent;
	struct TerrainComponent;
	struct PlaneComponent;

	class ProceduralMeshGenSystem : public core::System
	{
	public:

		ProceduralMeshGenSystem(const std::shared_ptr<core::Engine>& engine) : System(engine)
		{
			const auto registry = m_engine->get_system<ecs::EnTTSubsystem>()->registry();

			/*registry->on_construct<PlaneComponent>().connect<&ProceduralMeshGenSystem::onConstructPlane>();
			registry->on_update<PlaneComponent>().connect<&ProceduralMeshGenSystem::onConstructPlane>();

			registry->on_construct<TerrainComponent>().connect<&ProceduralMeshGenSystem::onConstructTerrain>();
			registry->on_update<TerrainComponent>().connect<&ProceduralMeshGenSystem::onConstructTerrain>();

			registry->on_construct<IcoSphereComponent>().connect<&ProceduralMeshGenSystem::onConstructIcoSphere>();
			registry->on_update<IcoSphereComponent>().connect<&ProceduralMeshGenSystem::onConstructIcoSphere>();*/
		}

		~ProceduralMeshGenSystem() override { m_engine = nullptr; }


		static void onConstructPlane(entt::registry& registry, entt::entity entity);
		static void onConstructTerrain(entt::registry& registry, entt::entity entity);
		static void onConstructIcoSphere(entt::registry& registry, entt::entity entity);

	private:

		// Generator list of vertices/indices for a flat plane
		static void generatePlaneVertices(const Vector2f& halfSize, const Vector2i& numQuads, rendering::ProceduralMeshComponent& mesh);

		static void generateTerrain(const TerrainComponent& terrain, rendering::ProceduralMeshComponent& mesh);

		static void generateIcoSphere(const IcoSphereComponent& sphere, rendering::ProceduralMeshComponent& mesh);

	};
}
