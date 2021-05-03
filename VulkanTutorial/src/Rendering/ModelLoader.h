#pragma once

#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <cereal/types/vector.hpp>
#include <cereal/archives/binary.hpp>

#include <iostream>
#include <fstream>
#include <filesystem>

#include <Components/Rendering/MeshComponent.h>

namespace Puffin
{
	namespace IO
	{
		// Not for use outside ModelLoader
		inline void ProcessNode(aiNode* node, std::vector<aiMesh*>* meshes, const aiScene* scene)
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

		inline void SaveMesh(Rendering::MeshComponent& meshComp)
		{
			// Initialize Output File Stream and Cereal Binary Archive
			std::ofstream os(meshComp.model_path, std::ios::binary);
			cereal::BinaryOutputArchive archive(os);

			archive(meshComp.vertices);
			archive(meshComp.indices);
		}

		inline void LoadMesh(Rendering::MeshComponent& meshComp)
		{
			std::ifstream is(meshComp.model_path, std::ios::binary);
			cereal::BinaryInputArchive archive(is);

			archive(meshComp.vertices);
			archive(meshComp.indices);
		}

		// Import Mesh to MeshComponent
		inline bool ImportMesh(const std::string model_path)
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
			Rendering::MeshComponent meshComp;
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

					vertex.color = { 1.0f, 1.0f, 1.0f };

					// Get Normal
					vertex.normal =
					{
						mesh->mNormals[j].x,
						mesh->mNormals[j].y,
						mesh->mNormals[j].z
					};

					// Calculate Tangent
					vertex.tangent =
					{
						mesh->mTangents[j].x,
						mesh->mTangents[j].y,
						mesh->mTangents[j].z
					};

					// Get Tex Coords
					vertex.uv =
					{
						mesh->mTextureCoords[0][j].x,
						mesh->mTextureCoords[0][j].y
					};

					meshComp.vertices.push_back(vertex);
				}

				// Iterate over faces in mesh object
				for (int j = 0; j < mesh->mNumFaces; j++)
				{
					const aiFace* face = &mesh->mFaces[j];

					// Store all indices of this face
					for (int k = 0; k < face->mNumIndices; k++)
					{
						meshComp.indices.push_back(face->mIndices[k]);
					}
				}
			}

			std::filesystem::path path(model_path);
			std::string import_path = "content/models/" + path.stem().string() + ".psm";

			meshComp.model_path = import_path;

			SaveMesh(meshComp);

			// Import was successful, return true
			return true;
		}
	}
}

#endif // !MODEL_LOADER_H