#include "Procedural/ProceduralMeshGenSystem.hpp"

#include "Engine/Engine.hpp"
#include "ECS/Entity.hpp"

#include "OpenSimplexNoise/OpenSimplexNoise.h"

#include "Components/TransformComponent.h"
#include "Components/Rendering/MeshComponent.h"
#include "Components/Procedural/ProceduralMeshComponent.hpp"

namespace puffin::Procedural
{
	void ProceduralMeshGenSystem::Setup()
	{
		PackedVector<ECS::EntityPtr> proceduralPlaneEntities;
		ECS::GetEntities<TransformComponent, Rendering::ProceduralMeshComponent, PlaneComponent>(m_world, proceduralPlaneEntities);
		for (const auto& entity : proceduralPlaneEntities)
		{
			auto& mesh = entity->GetComponent<Rendering::ProceduralMeshComponent>();
			auto& plane = entity->GetComponent<PlaneComponent>();

			GeneratePlaneVertices(plane.halfSize, plane.numQuads, mesh.vertices, mesh.indices);

			entity->SetComponentFlag<PlaneComponent, FlagDirty>(false);
		}

		PackedVector<ECS::EntityPtr> proceduralTerrainEntities;
		ECS::GetEntities<TransformComponent, Rendering::ProceduralMeshComponent, TerrainComponent>(m_world, proceduralTerrainEntities);
		for (const auto& entity : proceduralTerrainEntities)
		{
			auto& mesh = entity->GetComponent<Rendering::ProceduralMeshComponent>();
			auto& terrain = entity->GetComponent<TerrainComponent>();

			GeneratePlaneVertices(terrain.halfSize, terrain.numQuads, mesh.vertices, mesh.indices);
			GenerateTerrain(mesh.vertices, terrain.seed, terrain.heightMultiplier, terrain.frequency, terrain.octaves, terrain.frequencyMult);

			entity->SetComponentFlag<TerrainComponent, FlagDirty>(false);
		}

		PackedVector<ECS::EntityPtr> proceduralIcoSphereEntities;
		ECS::GetEntities<TransformComponent, Rendering::ProceduralMeshComponent, IcoSphereComponent>(m_world, proceduralIcoSphereEntities);
		for (const auto& entity : proceduralIcoSphereEntities)
		{
			auto& mesh = entity->GetComponent<Rendering::ProceduralMeshComponent>();
			auto& sphere = entity->GetComponent<IcoSphereComponent>();

			GenerateIcoSphere(mesh.vertices, mesh.indices, sphere.subdivisions);

			entity->SetComponentFlag<IcoSphereComponent, FlagDirty>(false);
		}
	}

	void ProceduralMeshGenSystem::Update()
	{
		PackedVector<ECS::EntityPtr> proceduralPlaneEntities;
		ECS::GetEntities<TransformComponent, Rendering::ProceduralMeshComponent, PlaneComponent>(m_world, proceduralPlaneEntities);
		for (const auto& entity : proceduralPlaneEntities)
		{
			auto& mesh = entity->GetComponent<Rendering::ProceduralMeshComponent>();
			auto& plane = entity->GetComponent<PlaneComponent>();

			if (entity->GetComponentFlag<PlaneComponent, FlagDirty>())
			{
				GeneratePlaneVertices(plane.halfSize, plane.numQuads, mesh.vertices, mesh.indices);

				entity->SetComponentFlag<PlaneComponent, FlagDirty>(false);
				entity->SetComponentFlag<Rendering::ProceduralMeshComponent, FlagDirty>(true);
			}

			if (entity->GetComponentFlag<PlaneComponent, FlagDeleted>())
			{
				entity->RemoveComponent<PlaneComponent>();
				entity->SetComponentFlag<Rendering::ProceduralMeshComponent, FlagDeleted>(true);
			}
		}

		PackedVector<ECS::EntityPtr> proceduralTerrainEntities;
		ECS::GetEntities<TransformComponent, Rendering::ProceduralMeshComponent, TerrainComponent>(m_world, proceduralTerrainEntities);
		for (const auto& entity : proceduralTerrainEntities)
		{
			auto& mesh = entity->GetComponent<Rendering::ProceduralMeshComponent>();
			auto& terrain = entity->GetComponent<TerrainComponent>();

			if (entity->GetComponentFlag<TerrainComponent, FlagDirty>())
			{
				GeneratePlaneVertices(terrain.halfSize, terrain.numQuads, mesh.vertices, mesh.indices);
				GenerateTerrain(mesh.vertices, terrain.seed, terrain.heightMultiplier, terrain.frequency, terrain.octaves, terrain.frequencyMult);

				entity->SetComponentFlag<TerrainComponent, FlagDirty>(false);
				entity->SetComponentFlag<Rendering::ProceduralMeshComponent, FlagDirty>(true);
			}

			if (entity->GetComponentFlag<TerrainComponent, FlagDeleted>())
			{
				entity->RemoveComponent<TerrainComponent>();
				entity->SetComponentFlag<Rendering::ProceduralMeshComponent, FlagDeleted>(true);
			}
		}

		PackedVector<ECS::EntityPtr> proceduralIcoSphereEntities;
		ECS::GetEntities<TransformComponent, Rendering::ProceduralMeshComponent, IcoSphereComponent>(m_world, proceduralIcoSphereEntities);
		for (const auto& entity : proceduralIcoSphereEntities)
		{
			auto& mesh = entity->GetComponent<Rendering::ProceduralMeshComponent>();
			auto& sphere = entity->GetComponent<IcoSphereComponent>();

			if (entity->GetComponentFlag<IcoSphereComponent, FlagDirty>())
			{
				GenerateIcoSphere(mesh.vertices, mesh.indices, sphere.subdivisions);

				entity->SetComponentFlag<IcoSphereComponent, FlagDirty>(false);
				entity->SetComponentFlag<Rendering::ProceduralMeshComponent, FlagDirty>(true);
			}

			if (entity->GetComponentFlag<IcoSphereComponent, FlagDeleted>())
			{
				entity->RemoveComponent<IcoSphereComponent>();
				entity->SetComponentFlag<Rendering::ProceduralMeshComponent, FlagDeleted>(true);
			}
		}
	}

	void ProceduralMeshGenSystem::GeneratePlaneVertices(const Vector2f& halfSize, const Vector2i& numQuads,
		std::vector<Rendering::VertexPNTV32>& vertices, std::vector<uint32_t>& indices)
	{
		vertices.clear();
		indices.clear();

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
		vertices.resize(numVerticesTotal);

		for (int y = 0; y < numVerticesY; y++)
		{
			for (int x = 0; x < numVerticesX; x++)
			{
				Rendering::VertexPNTV32& vertex = vertices[x + (y * numVerticesX)];
				vertex.pos = { ((float)x * quadSize.x) - halfSize.x, 0.0f, ((float)y * quadSize.y) - halfSize.y };
				vertex.normal = { 0.0f, 1.0f, 0.0f };
				vertex.tangent = { 1.0f, 0.0f, 0.0f};
				vertex.uv = { (float)x * uvOffset.x, (float)y * uvOffset.y};
			}
		}

		const int numQuadsTotal = numQuads.x * numQuads.y;
		indices.reserve(numQuadsTotal * 6);

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
				indices.push_back(topLeft);
				indices.push_back(topRight);
				indices.push_back(bottomLeft);

				// Add second triangle
				indices.push_back(topRight);
				indices.push_back(bottomRight);
				indices.push_back(bottomLeft);
			}
		}
	}

	void ProceduralMeshGenSystem::GenerateTerrain(std::vector<Rendering::VertexPNTV32>& vertices, const int64_t& seed,
	                                              const double& heightMultiplier, const double& startFrequency, const int& octaves,
	                                              const double& frequencyMultiplier)
	{
		const OpenSimplexNoise::Noise noise(seed);
		double frequency = startFrequency;
		double amplitude = 1.0f;
		double amplitudeSum = 0.0f;

		std::vector<double> noiseValues;
		noiseValues.resize(vertices.size());

		for (int i = 0; i < octaves; i++)
		{
			for (int n = 0; n < noiseValues.size(); n++)
			{
				double noiseVal = noise.eval(vertices[n].uv.x * frequency, vertices[n].uv.y * frequency);
				noiseValues[n] += ((noiseVal + 1.0) / 2.0) * amplitude;
			}

			frequency *= frequencyMultiplier;
			amplitudeSum += amplitude;
			amplitude /= frequencyMultiplier;
		}

		for (int v = 0; v < noiseValues.size(); v++)
		{
			vertices[v].pos.y = (noiseValues[v] / amplitudeSum) * heightMultiplier;
		}
	}

	void ProceduralMeshGenSystem::GenerateIcoSphere(std::vector<Rendering::VertexPNTV32>& vertices,
		std::vector<uint32_t>& indices, const int& subdivisions)
	{
		vertices.clear();
		indices.clear();

		std::vector<Vector3f> positions;
		Icosahedron::VertexPositions(positions);

		vertices.resize(positions.size());
		for (int i = 0; i < positions.size(); i++)
		{
			Rendering::VertexPNTV32& vertex = vertices[i];
			vertex.pos = static_cast<glm::vec3>(positions[i]);
			vertex.normal = static_cast<glm::vec3>(positions[i].Normalised());
			vertex.tangent = { 0.0f, 0.0f, 0.0f };
			vertex.uv = { 0.0f, 0.0f };
		}

		Icosahedron::Indices(indices);
	}
}
