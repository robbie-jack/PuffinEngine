#pragma once

#ifndef MESH_ASSET_H
#define MESH_ASSET_H

#include <AssetRegistry.h>
#include <Components/Rendering/MeshComponent.h>

#include <vector>

namespace Puffin
{
	namespace IO
	{
		static bool ImportMesh(fs::path model_path);

		class StaticMeshAsset : public Asset
		{
		public:

			StaticMeshAsset() : Asset(fs::path()) {};

			StaticMeshAsset(fs::path path) : Asset(path) {};

			std::string Type();

			void Save();

			void Load();

			void Unload();

			std::vector<Rendering::Vertex> vertices_;
			std::vector<uint32_t> indices_;

			template<class Archive>
			void serialize(Archive& archive) const
			{
				archive(cereal::base_class<Asset>(this));
			}
		};
	}
}



#endif // !MESH_ASSET_H