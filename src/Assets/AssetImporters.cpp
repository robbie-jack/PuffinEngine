#include "Assets/AssetImporters.hpp"

#define TINYGLTF_IMPLEMENTATION

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.

#include <iostream>
#include <fstream>
#include <string>

#include "tiny_gltf.h"

#include "Assets/TextureAsset.h"
#include "Types/Vertex.hpp"
#include "Assets/MeshAsset.h"

namespace Puffin::IO
{
	//////////////////////
	// Model Importers
	//////////////////////
	
	bool LoadAndImportModel(const fs::path& modelPath)
	{
		if (modelPath.extension() == ".gltf")
			return LoadAndImportGLTFModel(modelPath);

		return false;
	}

	bool LoadGLTFModel(tinygltf::Model& model, const fs::path& modelPath)
	{
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		std::cout << "Loading glTF: " << modelPath << std::endl;

		bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, modelPath.string());

		if (!warn.empty())
		{
			std::cout << "WARN: " << warn << std::endl;
		}

		if (!err.empty())
		{
			std::cout << "ERR: " << err << std::endl;
		}

		if (!ret)
			std::cout << "Failed to load glTF: " << modelPath << std::endl;
		else
			std::cout << "Loaded glTF: " << modelPath << std::endl;

		return ret;
	}

	bool ImportGLTFModel(const tinygltf::Model& model, const fs::path& modelPath)
	{
		// Get buffer file paths
		std::vector<fs::path> bufferPaths;
		bufferPaths.resize(model.buffers.size());

		int i = 0;
		for (const auto& buffer : model.buffers)
		{
			if (buffer.uri.substr(0, 4) != "data")
			{
				bufferPaths[i] = modelPath.parent_path().string() + "//" + buffer.uri;
			}

			i++;
		}

		std::vector<Rendering::VertexPNTV32> vertices;
		std::vector<uint32_t> indices;

		// Import each mesh in file
		for (const auto& mesh : model.meshes)
		{
			if (mesh.primitives.size() == 0)
				continue;

			const auto& primitive = mesh.primitives[0];

			size_t vertexCount = 0;

			std::vector<char> binaryData;

			std::vector<Vector3f> vertexPos;
			std::vector<Vector3f> vertexNormal;
			std::vector<Vector2f> vertexUV;

			// Load Vertices
			for (const auto& attribute : primitive.attributes)
			{
				if (attribute.first == "POSITION")
				{
					const auto& accessor = model.accessors[attribute.second];

					if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC3)
					{
						const auto& bufferView = model.bufferViews[accessor.bufferView];
						vertexCount = accessor.count;

						LoadBinaryData(bufferPaths[bufferView.buffer], bufferView.byteOffset, bufferView.byteLength, binaryData);

						vertexPos.resize(vertexCount);
						memcpy(vertexPos.data(), binaryData.data(), vertexCount);
					}

					continue;
				}

				if (attribute.first == "NORMAL")
				{
					const auto& accessor = model.accessors[attribute.second];

					if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC3)
					{
						const auto& bufferView = model.bufferViews[accessor.bufferView];
						vertexCount = accessor.count;

						LoadBinaryData(bufferPaths[bufferView.buffer], bufferView.byteOffset, bufferView.byteLength, binaryData);

						vertexNormal.resize(vertexCount);
						memcpy(vertexNormal.data(), binaryData.data(), vertexCount);
					}

					continue;
				}

				if (attribute.first == "TEXCOORD_0")
				{
					const auto& accessor = model.accessors[attribute.second];

					if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC2)
					{
						const auto& bufferView = model.bufferViews[accessor.bufferView];
						vertexCount = accessor.count;

						LoadBinaryData(bufferPaths[bufferView.buffer], bufferView.byteOffset, bufferView.byteLength, binaryData);

						vertexUV.resize(vertexCount);
						memcpy(vertexUV.data(), binaryData.data(), vertexCount);
					}

					continue;
				}
			}

			// Copy data into vertices vector
			vertices.resize(vertexCount);
			for (int i = 0; i < vertexCount; i++)
			{
				vertices[i].pos = vertexPos[i];
				vertices[i].normal = vertexNormal[i];
				vertices[i].uv = vertexUV[i];
			}

			// Load Indices
			const auto& accessor = model.accessors[primitive.indices];

			size_t indexCount = 0;
			std::vector<unsigned short> indexShort;

			if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT && accessor.type == TINYGLTF_TYPE_SCALAR)
			{
				const auto& bufferView = model.bufferViews[accessor.bufferView];
				indexCount = accessor.count;

				LoadBinaryData(bufferPaths[bufferView.buffer], bufferView.byteOffset, bufferView.byteLength, binaryData);

				indexShort.resize(indexCount);
				memcpy(indexShort.data(), binaryData.data(), indexCount);
			}

			// Copy indices into vector
			indices.resize(indexCount);
			for (int i = 0; i < indexCount; i++)
			{
				indices[i] = indexShort[i];
			}

			fs::path assetPath = modelPath.parent_path().stem() / mesh.name;
			assetPath += ".pstaticmesh";

			Assets::MeshInfo info;
			info.compressionMode = Assets::CompressionMode::LZ4;
			info.originalFile = modelPath.string();
			info.vertexFormat = Rendering::VertexFormat::PNTV32;
			info.numVertices = vertices.size();
			info.numIndices = indices.size();
			info.verticesSize = vertices.size() * sizeof(Rendering::VertexPNTV32);
			info.indicesSize = indices.size() * sizeof(uint32_t);

			auto asset = Assets::AssetRegistry::Get()->AddAsset<Assets::StaticMeshAsset>(assetPath);

			if (!asset->Save(info, vertices.data(), indices.data()))
				return false;

			vertices.clear();
			indices.clear();
		}

		// Import Textures
		for (const auto& image : model.images)
		{
			fs::path texturePath = modelPath.parent_path() / image.uri;

			if (!LoadAndImportTexture(texturePath))
				return false;
		}

		return true;
	}

	bool LoadAndImportGLTFModel(const fs::path& modelPath)
	{
		tinygltf::Model model;

		if (!LoadGLTFModel(model, modelPath))
			return false;

		return ImportGLTFModel(model, modelPath);
	}

	bool LoadBinaryData(const fs::path& binaryPath, const int& byteOffset, const int& byteLength, std::vector<char>& binaryData)
	{
		// Open File for Loading
		std::ifstream binaryFile;
		binaryFile.open(binaryPath.c_str(), std::ios::binary);

		// Return false if file does not exist
		if (!binaryFile.is_open())
			return false;

		// Seek to start of binary blob
		binaryFile.seekg(byteOffset);

		// Load binary data
		binaryData.resize(byteLength);
		binaryFile.read(binaryData.data(), byteLength);

		binaryFile.close();

		return true;
	}

	//////////////////////
	// Texture Importers
	//////////////////////

	bool LoadAndImportTexture(fs::path texturePath)
	{
		int texWidth, texHeight, texChannels;

		stbi_uc* pixels = stbi_load(texturePath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels) {
			std::cout << "Failed to load texture file " << texturePath.string() << std::endl;
			return false;
		}

		void* pixelPtr = pixels;

		// Instantiate new Texture Asset to store loaded Pixel data
		fs::path importPath = texturePath.parent_path().stem() / texturePath.stem();
		importPath += ".ptexture";

		Assets::TextureInfo info;
		info.compressionMode = Assets::CompressionMode::LZ4;
		info.originalFile = texturePath.string();
		info.textureFormat = Assets::TextureFormat::RGBA8;
		info.textureHeight = (uint32_t)texHeight;
		info.textureWidth = (uint32_t)texWidth;
		info.originalSize = info.textureHeight * info.textureWidth * texChannels;

		auto asset = Assets::AssetRegistry::Get()->AddAsset<Assets::TextureAsset>(importPath);
		const bool ret = asset->Save(info, pixelPtr);

		// Free Loaded Data, as pixels are now in staging buffer
		stbi_image_free(pixels);

		return ret;
	}
}
