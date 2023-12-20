#pragma once

#include "ECS/EnTTSubsystem.h"
#include "Core/Engine.h"
#include "entt/entity/registry.hpp"
#include "Types/Vector.h"
#include "Core/System.h"

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

		ProceduralMeshGenSystem()
		{
			mSystemInfo.name = "ProceduralMeshGenSystem";
		}

		~ProceduralMeshGenSystem() override = default;

		void setup() override
		{
			//mEngine->registerCallback(core::ExecutionStage::Setup, [&]() { setup(); }, "ProcMeshGenSystem: Setup");
			//mEngine->registerCallback(core::ExecutionStage::Update, [&]() { update(); }, "ProcMeshGenSystem: Update", 200);

			const auto registry = mEngine->getSubsystem<ecs::EnTTSubsystem>()->registry();

			/*registry->on_construct<PlaneComponent>().connect<&ProceduralMeshGenSystem::onConstructPlane>();
			registry->on_update<PlaneComponent>().connect<&ProceduralMeshGenSystem::onConstructPlane>();

			registry->on_construct<TerrainComponent>().connect<&ProceduralMeshGenSystem::onConstructTerrain>();
			registry->on_update<TerrainComponent>().connect<&ProceduralMeshGenSystem::onConstructTerrain>();

			registry->on_construct<IcoSphereComponent>().connect<&ProceduralMeshGenSystem::onConstructIcoSphere>();
			registry->on_update<IcoSphereComponent>().connect<&ProceduralMeshGenSystem::onConstructIcoSphere>();*/
		}

		//void setup() const;
		//void update() const;

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
