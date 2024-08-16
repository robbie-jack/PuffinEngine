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
	struct IcoSphereComponent;
	struct TerrainComponent;
	struct ProceduralPlaneComponent;

	class ProceduralMeshGenSystem : public core::Subsystem
	{
	public:

		explicit ProceduralMeshGenSystem(const std::shared_ptr<core::Engine>& engine);
		~ProceduralMeshGenSystem() override;

		static void on_construct_plane(entt::registry& registry, entt::entity entity);
		static void on_construct_terrain(entt::registry& registry, entt::entity entity);
		static void on_construct_ico_sphere(entt::registry& registry, entt::entity entity);

	private:

		// Generator list of vertices/indices for a flat plane
		static void generate_plane_vertices(const Vector2f& half_size, const Vector2i& num_quads, rendering::ProceduralMeshComponent3D& mesh);

		static void generate_terrain(const TerrainComponent& terrain, rendering::ProceduralMeshComponent3D& mesh);

		static void generate_ico_sphere(const IcoSphereComponent& sphere, rendering::ProceduralMeshComponent3D& mesh);

	};
}
