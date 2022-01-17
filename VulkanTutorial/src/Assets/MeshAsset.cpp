#include "MeshAsset.h"

#include <AssetRegistry.h>

#include <cereal/types/vector.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/binary.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <fstream>

namespace Puffin
{
	namespace IO
	{
		////////////////////////////////
		// StaticMeshAsset
		////////////////////////////////

		std::string StaticMeshAsset::Type()
		{
			return "StaticMeshAsset";
		}

		bool StaticMeshAsset::Save()
		{
			fs::path fullPath = AssetRegistry::Get()->ContentRoot() / path_;
			const std::string string = fullPath.string();
			std::ofstream os(string, std::ios::binary);
			cereal::BinaryOutputArchive archive(os);

			const int numVertices = vertices_.size();
			const int numIndices = indices_.size();

			// Save Number of Vertices/Indices in human readable format
			archive(numVertices);
			archive(numIndices);

			// Save Vertex/Index Data in Base64 Encoded Binary
			archive(vertices_);
			archive(indices_);

			return true;
		}

		bool StaticMeshAsset::Load()
		{
			fs::path fullPath = AssetRegistry::Get()->ContentRoot() / path_;
			if (!fs::exists(fullPath))
				return false;

			const std::string string = fullPath.string();
			std::ifstream is(string, std::ios::binary);
			cereal::BinaryInputArchive archive(is);

			int numVertices, numIndices;

			// Load Number of Vertices/Indices
			archive(numVertices);
			archive(numIndices);

			vertices_.resize(numVertices);
			indices_.resize(numIndices);

			// Load Vertex/Index Data
			archive(vertices_);
			archive(indices_);

			return true;
		}

		void StaticMeshAsset::Unload()
		{
			vertices_.clear();
			vertices_.shrink_to_fit();

			indices_.clear();
			indices_.shrink_to_fit();
		}
	}
}

CEREAL_REGISTER_TYPE_WITH_NAME(Puffin::IO::StaticMeshAsset, "StaticMeshAsset");