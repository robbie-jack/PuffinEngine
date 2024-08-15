#include "puffin/procedural/proceduralmeshgensubsystem.h"

#include "puffin/ecs/enttsubsystem.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/components/procedural/proceduralmeshcomponent.h"
#include "puffin/components/rendering/meshcomponent.h"
#include "OpenSimplexNoise/OpenSimplexNoise.h"

namespace puffin::procedural
{
	ProceduralMeshGenSystem::ProceduralMeshGenSystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		m_name = "ProceduralMeshGenSubsystem";

		auto entt_subsystem = m_engine->get_subsystem<ecs::EnTTSubsystem>();
		const auto registry = entt_subsystem->registry();

		/*registry->on_construct<PlaneComponent>().connect<&ProceduralMeshGenSystem::onConstructPlane>();
		registry->on_update<PlaneComponent>().connect<&ProceduralMeshGenSystem::onConstructPlane>();

		registry->on_construct<TerrainComponent>().connect<&ProceduralMeshGenSystem::onConstructTerrain>();
		registry->on_update<TerrainComponent>().connect<&ProceduralMeshGenSystem::onConstructTerrain>();

		registry->on_construct<IcoSphereComponent>().connect<&ProceduralMeshGenSystem::onConstructIcoSphere>();
		registry->on_update<IcoSphereComponent>().connect<&ProceduralMeshGenSystem::onConstructIcoSphere>();*/
	}

	ProceduralMeshGenSystem::~ProceduralMeshGenSystem()
	{
		m_engine = nullptr;
	}

	void ProceduralMeshGenSystem::on_construct_plane(entt::registry& registry, entt::entity entity)
	{
		const auto& plane = registry.get<const PlaneComponent>(entity);
		auto& mesh = registry.get_or_emplace<rendering::ProceduralMeshComponent>(entity);

		generate_plane_vertices(plane.half_size, plane.num_quads, mesh);
	}

	void ProceduralMeshGenSystem::on_construct_terrain(entt::registry& registry, entt::entity entity)
	{
		const auto& terrain = registry.get<const TerrainComponent>(entity);
		auto& mesh = registry.get_or_emplace<rendering::ProceduralMeshComponent>(entity);

		generate_plane_vertices(terrain.half_size, terrain.num_quads, mesh);
		generate_terrain(terrain, mesh);
	}

	void ProceduralMeshGenSystem::on_construct_ico_sphere(entt::registry& registry, entt::entity entity)
	{
		const auto& sphere = registry.get<const IcoSphereComponent>(entity);
		auto& mesh = registry.get_or_emplace<rendering::ProceduralMeshComponent>(entity);

		generate_ico_sphere(sphere, mesh);
	}

	void ProceduralMeshGenSystem::generate_plane_vertices(const Vector2f& half_size, const Vector2i& num_quads, rendering::ProceduralMeshComponent& mesh)
	{
		mesh.vertices.clear();
		mesh.indices.clear();

		const Vector2f fullSize = half_size * 2.0f; // Get full size of plane

		// Get size of each quad
		Vector2f quadSize = fullSize;
		quadSize.x /= static_cast<float>(num_quads.x);
		quadSize.y /= static_cast<float>(num_quads.y);

		// Get UV coordinate offset for each vertex between 0 and 1
		Vector2f uvOffset = {1.0f};
		uvOffset.x /= static_cast<float>(num_quads.x);
		uvOffset.y /= static_cast<float>(num_quads.y);

		// Generate all vertices for plane
		const int numVerticesX = num_quads.x + 1;
		const int numVerticesY = num_quads.y + 1;
		const int numVerticesTotal = numVerticesX * numVerticesY;
		mesh.vertices.resize(numVerticesTotal);

		for (int y = 0; y < numVerticesY; y++)
		{
			for (int x = 0; x < numVerticesX; x++)
			{
				rendering::VertexPNTV32& vertex = mesh.vertices[x + (y * numVerticesX)];
				vertex.pos = { ((float)x * quadSize.x) - half_size.x, 0.0f, ((float)y * quadSize.y) - half_size.y };
				vertex.normal = { 0.0f, 1.0f, 0.0f };
				vertex.tangent = { 1.0f, 0.0f, 0.0f};
				vertex.uvX = (float)x * uvOffset.x;
				vertex.uvY = (float)y * uvOffset.y;
			}
		}

		const int numQuadsTotal = num_quads.x * num_quads.y;
		mesh.indices.reserve(numQuadsTotal * 6);

		// Generate indices for each quad/tri of plane
		for (int y = 0; y < num_quads.y; y++)
		{
			for (int x = 0; x < num_quads.y; x++)
			{
				// Calculate Indices for each corner of quad
				const int topLeft = x + (y * numVerticesX);
				const int topRight = (x + 1) + (y * numVerticesX);
				const int bottomLeft = x + ((y + 1) * numVerticesX);
				const int bottomRight = (x + 1) + ((y + 1) * numVerticesX);

				// Add first triangle
				mesh.indices.push_back(topLeft);
				mesh.indices.push_back(topRight);
				mesh.indices.push_back(bottomLeft);

				// Add second triangle
				mesh.indices.push_back(topRight);
				mesh.indices.push_back(bottomRight);
				mesh.indices.push_back(bottomLeft);
			}
		}
	}

	void ProceduralMeshGenSystem::generate_terrain(const TerrainComponent& terrain, rendering::ProceduralMeshComponent& mesh)
	{
		const OpenSimplexNoise::Noise noise(terrain.seed);
		double frequency = terrain.frequency;
		double amplitude = 1.0f;
		double amplitudeSum = 0.0f;

		std::vector<double> noiseValues;
		noiseValues.resize(mesh.vertices.size());

		for (int i = 0; i < terrain.octaves; i++)
		{
			for (int n = 0; n < noiseValues.size(); n++)
			{
				const double noiseVal = noise.eval(mesh.vertices[n].uvX * frequency, mesh.vertices[n].uvY * frequency);
				noiseValues[n] += ((noiseVal + 1.0) / 2.0) * amplitude;
			}

			frequency *= terrain.frequency_mult;
			amplitudeSum += amplitude;
			amplitude /= terrain.frequency_mult;
		}

		for (int v = 0; v < noiseValues.size(); v++)
		{
			mesh.vertices[v].pos.y = (noiseValues[v] / amplitudeSum) * terrain.height_multiplier;
		}
	}

	void ProceduralMeshGenSystem::generate_ico_sphere(const IcoSphereComponent& sphere, rendering::ProceduralMeshComponent& mesh)
	{
		mesh.vertices.clear();
		mesh.indices.clear();

		std::vector<Vector3f> positions;
		icosahedron::vertexPositions(positions);

		mesh.vertices.resize(positions.size());
		for (int i = 0; i < positions.size(); i++)
		{
			rendering::VertexPNTV32& vertex = mesh.vertices[i];
			vertex.pos = positions[i];
			vertex.normal = normalize(positions[i]);
			vertex.tangent = { 0.0f, 0.0f, 0.0f };
			vertex.uvX = 0.0f;
			vertex.uvY = 0.0f;
		}

		icosahedron::indices(mesh.indices);
	}
}
