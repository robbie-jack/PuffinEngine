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
			return "StaticMesh";
		}

		bool StaticMeshAsset::Save()
		{
			fs::path fullPath = AssetRegistry::Get()->ContentRoot() / RelativePath();
			const std::string string = fullPath.string();
			std::ofstream os(string, std::ios::binary);
			cereal::BinaryOutputArchive archive(os);

			const int numVertices = m_vertices.size();
			const int numIndices = m_indices.size();

			// Save Number of Vertices/Indices in human readable format
			archive(numVertices);
			archive(numIndices);

			// Save Vertex/Index Data in Base64 Encoded Binary
			archive(m_vertices);
			archive(m_indices);

			return true;
		}

		bool StaticMeshAsset::Load()
		{
			fs::path fullPath = AssetRegistry::Get()->ContentRoot() / RelativePath();
			if (!fs::exists(fullPath))
				return false;

			const std::string string = fullPath.string();
			std::ifstream is(string, std::ios::binary);
			cereal::BinaryInputArchive archive(is);

			int numVertices, numIndices;

			// Load Number of Vertices/Indices
			archive(numVertices);
			archive(numIndices);

			m_vertices.resize(numVertices);
			m_indices.resize(numIndices);

			// Load Vertex/Index Data
			archive(m_vertices);
			archive(m_indices);

			return true;
		}

		void StaticMeshAsset::Unload()
		{
			m_vertices.clear();
			m_vertices.shrink_to_fit();

			m_indices.clear();
			m_indices.shrink_to_fit();
		}
	}
}

CEREAL_REGISTER_TYPE_WITH_NAME(Puffin::IO::StaticMeshAsset, "StaticMeshAsset");