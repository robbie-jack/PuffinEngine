#include "Procedural/ProceduralMeshGenSystem.hpp"

#include "Engine/Engine.hpp"
#include "ECS/Entity.h"

#include "Components/TransformComponent.h"

namespace Puffin::Rendering::Procedural
{
	ProceduralMeshGenSystem::ProceduralMeshGenSystem()
	{
		m_systemInfo.name = "ProceduralMeshGenSystem";
		m_systemInfo.updateOrder = Core::UpdateOrder::PreRender;
	}

	void ProceduralMeshGenSystem::PreStart()
	{
		std::vector<std::shared_ptr<ECS::Entity>> proceduralPlaneEntities;
		ECS::GetEntities<TransformComponent, ProceduralPlaneComponent>(m_world, proceduralPlaneEntities);
		for (const auto& entity : proceduralPlaneEntities)
		{
			auto& plane = entity->GetComponent<ProceduralPlaneComponent>();

			GeneratePlaneVertices(plane.halfSize, plane.numQuads, plane.vertices, plane.indices);
		}
	}

	void ProceduralMeshGenSystem::Update()
	{
		std::vector<std::shared_ptr<ECS::Entity>> proceduralPlaneEntities;
		ECS::GetEntities<TransformComponent, ProceduralPlaneComponent>(m_world, proceduralPlaneEntities);
		for (const auto& entity : proceduralPlaneEntities)
		{
			auto& plane = entity->GetComponent<ProceduralPlaneComponent>();

			if (entity->GetComponentFlag<ProceduralPlaneComponent, FlagDirty>())
			{
				GeneratePlaneVertices(plane.halfSize, plane.numQuads, plane.vertices, plane.indices);
			}

			if (entity->GetComponentFlag<ProceduralPlaneComponent, FlagDeleted>())
			{
				plane.vertices.clear();
				plane.indices.clear();
			}
		}
	}

	void ProceduralMeshGenSystem::GeneratePlaneVertices(const Vector2f& halfSize, const Vector2i& numQuads,
		std::vector<Vertex_PNTV_32>& vertices, std::vector<uint32_t>& indices)
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
				Vertex_PNTV_32& vertex = vertices[x + (y * numVerticesX)];
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
}
