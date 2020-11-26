#pragma once

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <unordered_map>

#include "MeshComponent.h"

namespace Puffin
{
	namespace IO
	{
		void LoadMesh(Rendering::MeshComponent& mesh, std::string model_path)
		{
			tinyobj::attrib_t attrib;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;
			std::string warn, err;

			if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str()))
			{
				throw std::runtime_error(warn + err);
			}

			std::unordered_map<Rendering::Vertex, uint32_t> uniqueVertices = {};

			std::vector<Rendering::Vertex> vertices;
			std::vector<uint32_t> indices;

			for (const auto& shape : shapes)
			{
				for (const auto& index : shape.mesh.indices)
				{
					Rendering::Vertex vertex = {};

					vertex.pos =
					{
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					};

					vertex.normal =
					{
						attrib.vertices[3 * index.normal_index + 0],
						attrib.vertices[3 * index.normal_index + 1],
						attrib.vertices[3 * index.normal_index + 2]
					};

					vertex.texCoord =
					{
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
					};

					vertex.color = { 1.0f, 1.0f, 1.0f };

					if (uniqueVertices.count(vertex) == 0)
					{
						uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
						vertices.push_back(vertex);
					}

					indices.push_back(uniqueVertices[vertex]);
				}
			}

			mesh.vertices = vertices;
			mesh.indices = indices;
		}

		// Not for use outside ModelLoader
		void ProcessNode(aiNode* node, std::vector<aiMesh*>* meshes, const aiScene* scene)
		{
			// Process all meshes in this node
			for (int i = 0; i < node->mNumMeshes; i++)
			{
				meshes->push_back(scene->mMeshes[node->mMeshes[i]]);
			}

			// Process all meshes in child nodes
			for (int i = 0; i < node->mNumChildren; i++)
			{
				ProcessNode(node->mChildren[i], meshes, scene);
			}
		}

		// Import Mesh to MeshComponent
		bool ImportMesh(Rendering::MeshComponent& meshComp, const std::string model_path)
		{
			// Create an Instance of the Assimp Importer Class
			Assimp::Importer importer;

			// Import Model
			const aiScene* scene = importer.ReadFile(model_path, 
				aiProcess_CalcTangentSpace		| // Calculate Tangents and Bitangents (Useful for certain shader effects)
				aiProcess_Triangulate			| // Ensure all faces are triangles
				aiProcess_JoinIdenticalVertices	| // Ensure all vertices are unique
				aiProcess_SortByPType			|
				aiProcess_FlipUVs				|
				aiProcess_GenNormals);

			// Check if Import Failed
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			{
				// Import Failed, Return false and print error message
				std::cout << "Failed to import model with Error: " << importer.GetErrorString() << std::endl;
				return false;
			}

			// Local vectors for storing model data
			std::vector<Rendering::Vertex> vertices;
			std::vector<uint32_t> indices;
			std::vector<aiMesh*> meshes;

			aiNode* root = scene->mRootNode;

			// Process all meshes in root node
			for (int i = 0; i < root->mNumChildren; i++)
			{
				ProcessNode(root->mChildren[i], &meshes, scene);
			}

			// Retrieve mesh data from scene file
			for (int i = 0; i < meshes.size(); i++)
			{
				// Get current aiMesh object
				const aiMesh* mesh = meshes[i];

				// Iterate over all imported vertices and fill vertex vector
				for (int j = 0; j < mesh->mNumVertices; j++)
				{
					Rendering::Vertex vertex = {};

					// Get Position
					vertex.pos =
					{
						mesh->mVertices[j].x,
						mesh->mVertices[j].y,
						mesh->mVertices[j].z
					};

					// Get Normal
					vertex.normal =
					{
						mesh->mNormals[j].x,
						mesh->mNormals[j].y,
						mesh->mNormals[j].z
					};

					// Get Tex Coords
					vertex.texCoord =
					{
						mesh->mTextureCoords[0][j].x,
						mesh->mTextureCoords[0][j].y
					};

					vertex.color = { 1.0f, 1.0f, 1.0f };

					vertices.push_back(vertex);
				}

				// Iterate over faces in mesh object
				for (int j = 0; j < mesh->mNumFaces; j++)
				{
					const aiFace* face = &mesh->mFaces[j];

					// Store all indices of this face
					for (int k = 0; k < face->mNumIndices; k++)
					{
						indices.push_back(face->mIndices[k]);
					}
				}
			}

			meshComp.vertices = vertices;
			meshComp.indices = indices;

			// Import was successful, return true
			return true;
		}
	}
}