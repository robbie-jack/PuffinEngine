#include "MeshAsset.h"
#include "nlohmann/json.hpp"
#include "lz4/lz4.h"

using json = nlohmann::json;

namespace Puffin::Assets
{
	////////////////////////////////
	// StaticMeshAsset
	////////////////////////////////

	bool StaticMeshAsset::Save(const std::vector<Rendering::Vertex>& vertices, const std::vector<uint32_t>& indices)
	{
		const fs::path fullPath = AssetRegistry::Get()->ContentRoot() / RelativePath();

		json meshMetadata;
		meshMetadata["vertex_format"] = "PNCTV_F32";
		meshMetadata["vertex_buffer_size"] = vertices.size();
		meshMetadata["index_buffer_size"] = indices.size();

		//size_t fullSize = 

		return true;
	}

	bool StaticMeshAsset::Load()
	{
		if (m_isLoaded)
			return true;

		const fs::path fullPath = AssetRegistry::Get()->ContentRoot() / RelativePath();
		if (!fs::exists(fullPath))
			return false;



		m_isLoaded = true;
		return true;
	}

	void StaticMeshAsset::Unload()
	{
		m_vertices.clear();
		m_vertices.shrink_to_fit();

		m_indices.clear();
		m_indices.shrink_to_fit();

		m_isLoaded = false;
	}
}