#include "Mesh.h"

namespace Puffin
{
	namespace Rendering
	{
		Mesh::Mesh()
		{
			transform.position = Puffin::Vector3(0.0f, 0.0f, 0.0f);
			transform.rotation = Puffin::Vector3(0.0f, 0.0f, 0.0f);
			transform.scale = Puffin::Vector3(1.0f, 1.0f, 1.0f);
		}

		Mesh::~Mesh()
		{

		}

		void Mesh::SetupMesh(std::vector<Vertex> vertices_, std::vector<uint32_t> indices_)
		{
			meshData.vertices = vertices_;
			meshData.indices = indices_;
		}

		void Mesh::SetColor(glm::vec3 color_)
		{
			for (int i = 0; i < meshData.vertices.size(); i++)
			{
				meshData.vertices[i].color = color_;
			}
		}
	}
}