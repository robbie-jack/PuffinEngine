#pragma once

#include "puffin/core/engine.h"
#include "entt/entity/registry.hpp"
#include "puffin/types/vector.h"
#include "puffin/core/subsystem.h"

namespace puffin
{
	namespace rendering
	{
		struct ProceduralMeshComponent3D;
	}
}

namespace puffin::procedural
{
	struct ProceduralIcoSphereComponent3D;
	struct ProceduralTerrainComponent3D;

	class ProceduralMeshGenSystem : public core::Subsystem
	{
	public:

		explicit ProceduralMeshGenSystem(const std::shared_ptr<core::Engine>& engine);
		~ProceduralMeshGenSystem() override;

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
