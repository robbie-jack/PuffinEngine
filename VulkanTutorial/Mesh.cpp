#include "Mesh.h"

Mesh::Mesh()
{

}

Mesh::~Mesh()
{

}

void Mesh::CreateMesh(std::vector<Vertex> vertices_, std::vector<uint32_t> indices_)
{
	vertices = vertices_;
	indices = indices_;
}