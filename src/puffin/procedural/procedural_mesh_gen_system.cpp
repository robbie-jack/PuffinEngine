#include "puffin/procedural/procedural_mesh_gen_system.h"

#include "puffin/components/transform_component_3d.h"
#include "puffin/components/procedural/procedural_mesh_component.h"
#include "puffin/components/rendering/mesh_component.h"
#include "OpenSimplexNoise/OpenSimplexNoise.h"

namespace puffin::procedural
{
	void ProceduralMeshGenSystem::onConstructPlane(entt::registry& registry, entt::entity entity)
	{
		const auto& plane = registry.get<const PlaneComponent>(entity);
		auto& mesh = registry.get_or_emplace<rendering::ProceduralMeshComponent>(entity);

		generatePlaneVertices(plane.halfSize, plane.numQuads, mesh);
	}

	void ProceduralMeshGenSystem::onConstructTerrain(entt::registry& registry, entt::entity entity)
	{
		const auto& terrain = registry.get<const TerrainComponent>(entity);
		auto& mesh = registry.get_or_emplace<rendering::ProceduralMeshComponent>(entity);

		generatePlaneVertices(terrain.halfSize, terrain.numQuads, mesh);
		generateTerrain(terrain, mesh);
	}

	void ProceduralMeshGenSystem::onConstructIcoSphere(entt::registry& registry, entt::entity entity)
	{
		const auto& sphere = registry.get<const IcoSphereComponent>(entity);
		auto& mesh = registry.get_or_emplace<rendering::ProceduralMeshComponent>(entity);

		generateIcoSphere(sphere, mesh);
	}

	void ProceduralMeshGenSystem::generatePlaneVertices(const Vector2f& halfSize, const Vector2i& numQuads, rendering::ProceduralMeshComponent& mesh)
	{
		mesh.vertices.clear();
		mesh.indices.clear();

		const Vector2f fullSize = halfSize * 2.0f; // Get full size of plane

		// Get size of each quad
		Vector2f quadSize = fullSize;
		quadSize.x /= static_cast<float>(numQuads.x);
		quadSize.y /= static_cast<float>(numQuads.y);

		// Get UV coordinate offset for each vertex between 0 and 1
		Vector2f uvOffset = {1.0f};
		uvOffset.x /= static_cast<float>(numQuads.x);
		uvOffset.y /= static_cast<float>(numQuads.y);

		// Generate all vertices for plane
		const int numVerticesX = numQuads.x + 1;
		const int numVerticesY = numQuads.y + 1;
		const int numVerticesTotal = numVerticesX * numVerticesY;
		mesh.vertices.resize(numVerticesTotal);

		for (int y = 0; y < numVerticesY; y++)
		{
			for (int x = 0; x < numVerticesX; x++)
			{
				rendering::VertexPNTV32& vertex = mesh.vertices[x + (y * numVerticesX)];
				vertex.pos = { ((float)x * quadSize.x) - halfSize.x, 0.0f, ((float)y * quadSize.y) - halfSize.y };
				vertex.normal = { 0.0f, 1.0f, 0.0f };
				vertex.tangent = { 1.0f, 0.0f, 0.0f};
				vertex.uvX = (float)x * uvOffset.x;
				vertex.uvY = (float)y * uvOffset.y;
			}
		}

		const int numQuadsTotal = numQuads.x * numQuads.y;
		mesh.indices.reserve(numQuadsTotal * 6);

		// Generate indices for each quad/tri of plane
		for (int y = 0; y < numQuads.y; y++)
		{
			for (int x = 0; x < numQuads.y; x++)
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

	void ProceduralMeshGenSystem::generateTerrain(const TerrainComponent& terrain, rendering::ProceduralMeshComponent& mesh)
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
			mesh.vertices[v].pos.y = (noiseValues[v] / amplitudeSum) * terrain.heightMultiplier;
		}
	}

	void ProceduralMeshGenSystem::generateIcoSphere(const IcoSphereComponent& sphere, rendering::ProceduralMeshComponent& mesh)
	{
		mesh.vertices.clear();
		mesh.indices.clear();

		std::vector<Vector3f> positions;
		icosahedron::vertexPositions(positions);

		mesh.vertices.resize(positions.size());
		for (int i = 0; i < positions.size(); i++)
		{
			rendering::VertexPNTV32& vertex = mesh.vertices[i];
			vertex.pos = static_cast<glm::vec3>(positions[i]);
			vertex.normal = static_cast<glm::vec3>(positions[i].normalized());
			vertex.tangent = { 0.0f, 0.0f, 0.0f };
			vertex.uvX = 0.0f;
			vertex.uvY = 0.0f;
		}

		icosahedron::indices(mesh.indices);
	}
}
