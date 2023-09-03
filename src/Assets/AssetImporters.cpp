#include "Assets/AssetImporters.h"

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
#include "Types/Vertex.h"
#include "Assets/MeshAsset.h"

namespace puffin::io
{
	//////////////////////
	// Model Importers
	//////////////////////
	
	bool loadAndImportModel(const fs::path& modelPath)
	{
		if (modelPath.extension() == ".gltf")
			return loadAndImportGltfModel(modelPath);

		return false;
	}

	bool loadGltfModel(tinygltf::Model& model, const fs::path& modelPath)
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

	bool importGltfModel(const tinygltf::Model& model, const fs::path& modelPath)
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

		std::vector<rendering::VertexPNTV32> vertices;
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

						loadBinaryData(bufferPaths[bufferView.buffer], bufferView.byteOffset, bufferView.byteLength, binaryData);

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

						loadBinaryData(bufferPaths[bufferView.buffer], bufferView.byteOffset, bufferView.byteLength, binaryData);

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

						loadBinaryData(bufferPaths[bufferView.buffer], bufferView.byteOffset, bufferView.byteLength, binaryData);

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

				loadBinaryData(bufferPaths[bufferView.buffer], bufferView.byteOffset, bufferView.byteLength, binaryData);

				indexShort.resize(indexCount);
				memcpy(indexShort.data(), binaryData.data(), indexCount);
			}

			// Copy indices into vector
			indices.resize(indexCount);
			for (int i = 0; i < indexCount; i++)
			{
				indices[i] = indexShort[i];
			}

			std::vector<Vector3f> tan;
			tan.resize(vertexCount);

			// Generate Tangents
			for (int i = 0; i < indexCount; i += 3)
			{
				uint32_t i1 = indices[i];
				uint32_t i2 = indices[i + 1];
				uint32_t i3 = indices[i + 2];

				const Vector3f& v1 = vertices[i1].pos;
				const Vector3f& v2 = vertices[i2].pos;
				const Vector3f& v3 = vertices[i3].pos;

				const Vector2f& uv1 = vertices[i1].uv;
				const Vector2f& uv2 = vertices[i2].uv;
				const Vector2f& uv3 = vertices[i3].uv;

				float x1 = v2.x - v1.x;
				float x2 = v3.x - v1.x;
				float y1 = v2.y - v1.y;
				float y2 = v3.y - v1.y;
				float z1 = v2.z - v1.z;
				float z2 = v3.z - v1.z;

				float s1 = uv2.x - uv1.x;
				float s2 = uv3.x - uv1.x;
				float t1 = uv2.y - uv1.y;
				float t2 = uv3.y - uv1.y;

				float r = 1.0F / (s1 * t2 - s2 * t1);

				Vector3f sdir((t2* x1 - t1 * x2)* r, (t2* y1 - t1 * y2)* r,
					(t2* z1 - t1 * z2)* r);

				/*Vector3f tdir((s1* x2 - s2 * x1)* r, (s1* y2 - s2 * y1)* r,
					(s1* z2 - s2 * z1)* r);*/

				tan[i1] += sdir;
				tan[i2] += sdir;
				tan[i3] += sdir;
			}

			for (int i = 0; i < vertexCount; i++)
			{
				const Vector3f& n = vertices[i].normal;
				const Vector3f& t = tan[i];

				vertices[i].tangent = (t - n * n.dot(t)).normalized();
			}

			fs::path assetPath = modelPath.parent_path().stem() / mesh.name;
			assetPath += ".pstaticmesh";

			assets::MeshInfo info;
			info.compressionMode = assets::CompressionMode::LZ4;
			info.originalFile = modelPath.string();
			info.vertexFormat = rendering::VertexFormat::PNTV32;
			info.numVertices = vertices.size();
			info.numIndices = indices.size();
			info.verticesSize = vertices.size() * sizeof(rendering::VertexPNTV32);
			info.indicesSize = indices.size() * sizeof(uint32_t);

			auto asset = assets::AssetRegistry::get()->addAsset<assets::StaticMeshAsset>(assetPath);

			if (!asset->save(info, vertices.data(), indices.data()))
				return false;

			vertices.clear();
			indices.clear();
		}

		// Import Textures
		for (const auto& image : model.images)
		{
			fs::path texturePath = modelPath.parent_path() / image.uri;

			if (!loadAndImportTexture(texturePath))
				return false;
		}

		return true;
	}

	bool loadAndImportGltfModel(const fs::path& modelPath)
	{
		tinygltf::Model model;

		if (!loadGltfModel(model, modelPath))
			return false;

		return importGltfModel(model, modelPath);
	}

	bool loadBinaryData(const fs::path& binaryPath, const int& byteOffset, const int& byteLength, std::vector<char>& binaryData)
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

	bool loadAndImportTexture(fs::path texturePath, fs::path assetSubdirectory, bool useBCFormat)
	{
		int texWidth, texHeight, texChannels;

		// Load info about texture without loading full file
		if (const int ok = stbi_info(texturePath.string().c_str(), &texWidth, &texHeight, &texChannels); ok == 0)
		{
			std::cout << "Failed to load texture file " << texturePath.string() << std::endl;
			return false;
		}

		// Figure out how many channels loaded pixel data should have (rgb data gets an extra channel since it's stored the same as rgba data)
		int desiredChannels;
		if (texChannels == 3)
		{
			desiredChannels = 4;
		}
		else
		{
			desiredChannels = texChannels;
		}

		// Load pixel data
		stbi_uc* pixels = stbi_load(texturePath.string().c_str(), &texWidth, &texHeight, &texChannels, desiredChannels);

		if (!pixels) {
			std::cout << "Failed to load texture file " << texturePath.string() << std::endl;
			return false;
		}

		void* pixelPtr = pixels;

		// Instantiate new Texture Asset to store loaded Pixel data
		fs::path assetPath = assetSubdirectory / texturePath.stem();
		assetPath += ".ptexture";

		assets::TextureInfo info;
		info.compressionMode = assets::CompressionMode::LZ4;
		info.originalFile = texturePath.string();
		info.textureHeight = static_cast<uint32_t>(texHeight);
		info.textureWidth = static_cast<uint32_t>(texWidth);
		info.textureChannels = static_cast<uint8_t>(desiredChannels);
		info.originalSize = info.textureHeight * info.textureWidth * info.textureChannels;

		// Decide which texture format to use
		if (useBCFormat)
		{
			info.textureFormat = assets::gTexChannelsToBCFormat.at(info.textureChannels);
		}
		else
		{
			info.textureFormat = assets::gTexChannelsToRGBAFormat.at(info.textureChannels);
		}

		const auto asset = assets::AssetRegistry::get()->addAsset<assets::TextureAsset>(assetPath);
		const bool ret = asset->save(info, pixelPtr);

		// Free Loaded Data, as pixels are now in staging buffer
		stbi_image_free(pixels);

		return ret;
	}
}
