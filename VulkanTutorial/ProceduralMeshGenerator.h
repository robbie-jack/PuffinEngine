#pragma once

#include "Mesh.h"
#include "Vector.h"

namespace Puffin
{
	namespace Rendering
	{
		namespace Procedural
		{
			static int GetIndexForGridCoordinates(int x, int y, int segments)
			{
				return x + y * segments;
			}

			// Generate a Plane Mesh made up of multiple segments
			static void GeneratePlaneMesh(Mesh& mesh, int XSegments, int YSegments, float SegmentSize)
			{
				Vector3 Origin = Vector3(-(XSegments * SegmentSize) / 2, -(YSegments * SegmentSize) / 2, 0.0f);
				int XVertices = XSegments + 1;
				int YVertices = YSegments + 1;

				std::vector<Vertex> vertices;
				std::vector<uint32_t> indices;

				// Generate Vertices
				for (int y = 0; y < YVertices; y++)
				{
					for (int x = 0; x < XVertices; x++)
					{
						Vertex vertex0, vertex1, vertex2, vertex3;
						Vector3 QuadOrigin = Origin + Vector3(x * SegmentSize, y * SegmentSize, 0.0f);
						Vector2 texCoord = Vector2(1 / XVertices * x, 1 / YVertices * y);

						// Top Left Vertex
						vertex0.pos = QuadOrigin;
						vertex0.normal = Vector3(0.0f, 0.0f, 1.0f);
						vertex0.color = Vector3(1.0f, 1.0f, 1.0f);
						vertex0.texCoord = texCoord;

						vertices.push_back(vertex0);
					}
				}

				// Generate Indices
				for (int y = 0; y < YSegments; y++)
				{
					for (int x = 0; x < XSegments; x++)
					{
						int top_left_index = GetIndexForGridCoordinates(x, y, XSegments);
						int bottom_left_index = GetIndexForGridCoordinates(x, y + 1, XSegments);
						int bottom_right_index = GetIndexForGridCoordinates(x + 1, y + 1, XSegments);
						int top_right_index = GetIndexForGridCoordinates(x + 1, y, XSegments);

						indices.push_back(top_left_index);
						indices.push_back(bottom_left_index);
						indices.push_back(bottom_right_index);
						indices.push_back(top_left_index);
						indices.push_back(bottom_right_index);
						indices.push_back(top_right_index);
					}
				}

				mesh.SetupMesh(vertices, indices);
			}
		}
	}
}