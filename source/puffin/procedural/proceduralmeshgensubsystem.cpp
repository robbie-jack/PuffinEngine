#include "puffin/procedural/proceduralmeshgensubsystem.h"

#include "OpenSimplexNoise/OpenSimplexNoise.h"

#include "puffin/ecs/enttsubsystem.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/components/procedural/3d/proceduralicospherecomponent3d.h"
#include "puffin/components/rendering/3d/proceduralmeshcomponent3d.h"

#include "puffin/components/procedural/3d/proceduralplanecomponent3d.h"
#include "puffin/components/procedural/3d/proceduralterraincomponent3d.h"

namespace puffin::procedural
{
	ProceduralMeshGenSystem::ProceduralMeshGenSystem(const std::shared_ptr<core::Engine>& engine) : Subsystem(engine)
	{
		mName = "ProceduralMeshGenSubsystem";

		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->registry();

		/*registry->on_construct<PlaneComponent>().connect<&ProceduralMeshGenSystem::onConstructPlane>();
		registry->on_update<PlaneComponent>().connect<&ProceduralMeshGenSystem::onConstructPlane>();

		registry->on_construct<TerrainComponent>().connect<&ProceduralMeshGenSystem::onConstructTerrain>();
		registry->on_update<TerrainComponent>().connect<&ProceduralMeshGenSystem::onConstructTerrain>();

		registry->on_construct<IcoSphereComponent>().connect<&ProceduralMeshGenSystem::onConstructIcoSphere>();
		registry->on_update<IcoSphereComponent>().connect<&ProceduralMeshGenSystem::onConstructIcoSphere>();*/
	}

	ProceduralMeshGenSystem::~ProceduralMeshGenSystem()
	{
		mEngine = nullptr;
	}

	void ProceduralMeshGenSystem::OnConstructPlane(entt::registry& registry, entt::entity entity)
	{
		const auto& plane = registry.get<const ProceduralPlaneComponent3D>(entity);
		auto& mesh = registry.get_or_emplace<rendering::ProceduralMeshComponent3D>(entity);

		GeneratePlaneVertices(plane.halfSize, plane.quadCount, mesh);
	}

	void ProceduralMeshGenSystem::OnConstructTerrain(entt::registry& registry, entt::entity entity)
	{
		const auto& terrain = registry.get<const ProceduralTerrainComponent3D>(entity);
		auto& mesh = registry.get_or_emplace<rendering::ProceduralMeshComponent3D>(entity);

		GeneratePlaneVertices(terrain.halfSize, terrain.quadCount, mesh);
		GenerateTerrain(terrain, mesh);
	}

	void ProceduralMeshGenSystem::OnConstructIcoSphere(entt::registry& registry, entt::entity entity)
	{
		const auto& sphere = registry.get<const ProceduralIcoSphereComponent3D>(entity);
		auto& mesh = registry.get_or_emplace<rendering::ProceduralMeshComponent3D>(entity);

		GenerateIcoSphere(sphere, mesh);
	}

	void ProceduralMeshGenSystem::GeneratePlaneVertices(const Vector2f& half_size, const Vector2i& num_quads, rendering::ProceduralMeshComponent3D& mesh)
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

	void ProceduralMeshGenSystem::GenerateTerrain(const ProceduralTerrainComponent3D& terrain,
		rendering::ProceduralMeshComponent3D& mesh)
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

			frequency *= terrain.frequencyMult;
			amplitudeSum += amplitude;
			amplitude /= terrain.frequencyMult;
		}

		for (int v = 0; v < noiseValues.size(); v++)
		{
			mesh.vertices[v].pos.y = (noiseValues[v] / amplitudeSum) * terrain.heightMult;
		}
	}

	void ProceduralMeshGenSystem::GenerateIcoSphere(const ProceduralIcoSphereComponent3D& sphere,
		rendering::ProceduralMeshComponent3D& mesh)
	{
		mesh.vertices.clear();
		mesh.indices.clear();

		std::vector<Vector3f> positions;
		icosahedron::VertexPositions(positions);

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

		icosahedron::Indices(mesh.indices);
	}
}
