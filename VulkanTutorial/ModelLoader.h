#pragma once

#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>

#include "MeshComponent.h"

namespace Puffin
{
	namespace IO
	{
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

#endif // !MODEL_LOADER_H