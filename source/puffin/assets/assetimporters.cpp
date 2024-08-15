#include "puffin/assets/assetimporters.h"

#define TINYGLTF_USE_CPP14

#define TINYGLTF_IMPLEMENTATION

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.

#define TINYOBJLOADER_IMPLEMENTATION

#include <iostream>
#include <fstream>
#include <string>

#include "tiny_gltf.h"

#ifdef PFN_PLATFORM_WIN32
#include "compressonator/compressonator.h"
#endif

#include "tinyobjloader/tiny_obj_loader.h"

#include "puffin/assets/asset.h"
#include "puffin/assets/assetregistry.h"
#include "puffin/assets/textureasset.h"
#include "puffin/assets/staticmeshasset.h"
#include "puffin/types/vertex.h"

namespace puffin::io
{
	void GenerateTangents(std::vector<rendering::VertexPNTV32>& vertices, std::vector<uint32_t>& indices)
	{
		std::vector<Vector3f> tan;
		tan.resize(vertices.size());

		// Generate Tangents
		for (int i = 0; i < indices.size(); i += 3)
		{
			uint32_t i1 = indices[i];
			uint32_t i2 = indices[i + 1];
			uint32_t i3 = indices[i + 2];

			const Vector3f& v1 = vertices[i1].pos;
			const Vector3f& v2 = vertices[i2].pos;
			const Vector3f& v3 = vertices[i3].pos;

			const Vector2f& uv1 = { vertices[i1].uvX, vertices[i1].uvY };
			const Vector2f& uv2 = { vertices[i2].uvX, vertices[i2].uvY };
			const Vector2f& uv3 = { vertices[i3].uvX, vertices[i3].uvY };

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

			Vector3f sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
				(t2 * z1 - t1 * z2) * r);

			/*Vector3f tdir((s1* x2 - s2 * x1)* r, (s1* y2 - s2 * y1)* r,
				(s1* z2 - s2 * z1)* r);*/

			tan[i1] += sdir;
			tan[i2] += sdir;
			tan[i3] += sdir;
		}

		for (int i = 0; i < vertices.size(); i++)
		{
			const Vector3f& n = vertices[i].normal;
			const Vector3f& t = tan[i];

			vertices[i].tangent = normalize(t - n * dot(n, t));
		}
	}

	void copy_gltf_buffer_data(const tinygltf::BufferView& bufferView, const tinygltf::Buffer& buffer, const size_t& expectedStride, void* dst)
	{
		const size_t byteOffset = bufferView.byteOffset;
		const size_t byteLength = bufferView.byteLength;
		const size_t byteStride = bufferView.byteStride;

		const auto dstChar = static_cast<char*>(dst);

		// Data is packed together, copy with a single command
		if (byteStride == 0 || byteStride == expectedStride)
		{
			memcpy(dstChar, buffer.data.data() + byteOffset, byteLength);
		}
		// Byte data is interleaved, copy each attribute individually
		else
		{
			int i = 0;
			for (size_t offset = byteOffset; offset < byteOffset + byteLength; offset += byteStride)
			{
				memcpy(dstChar + (i * byteStride), buffer.data.data() + offset, expectedStride);

				i++;
			}
		}
	}

	//////////////////////
	// Model Importers
	//////////////////////
	
	bool LoadAndImportModel(const fs::path& modelPath, fs::path assetSubdirectory)
	{
		if (modelPath.extension() == ".gltf" || modelPath.extension() == ".glb")
			return LoadAndImportGltfModel(modelPath, assetSubdirectory);

		if (modelPath.extension() == ".obj")
			return LoadAndImportObjModel(modelPath, assetSubdirectory);

		std::cout << "Failed to load mesh, unsupported format" << std::endl;

		return false;
	}

	// OBJ

	bool LoadAndImportObjModel(const fs::path& modelPath, fs::path assetSubdirectory)
	{
		// Load obj data into reader object
		tinyobj::ObjReaderConfig config;
		config.triangulate = true; // Triangulate meshes so all polygons only have three vertices
		config.vertex_color = false;

		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(modelPath.string(), config))
		{
			std::cout << "Failed to load OBJ: " << modelPath << std::endl;

			if (!reader.Error().empty())
			{
				std::cout << "TinyObjReader: " << reader.Error() << std::endl;
			}

			return false;
		}

		if (!reader.Warning().empty())
		{
			std::cout << "TinyObjReader: " << reader.Warning() << std::endl;
		}

		// Parse obj data into puffin vertex/index format
		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		//auto& materials = reader.GetMaterials();

		std::vector<rendering::VertexPNTV32> vertices;
		std::vector<uint32_t> indices;

		assets::MeshAssetInfo meshAssetInfo;

		meshAssetInfo.subMeshInfos.reserve(shapes.size());

		size_t indexCountTotal = 0;
		for (const auto& shape : shapes)
		{
			indexCountTotal += shape.mesh.indices.size();
		}

		vertices.reserve(indexCountTotal);
		indices.reserve(indexCountTotal);

		// Loop over shapes
		int s = 0;
		for (const auto& shape : shapes)
		{
			rendering::VertexPNTV32 vertex;

			assets::SubMeshInfo subMeshInfo;
			subMeshInfo.vertexOffset = vertices.size();
			subMeshInfo.indexOffset = indices.size();

			std::unordered_map<rendering::VertexPNTV32, uint32_t> vertexIndices;

			// Loop over faces (polygon) to get indices
			for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
			{
				auto fv = static_cast<size_t>(shape.mesh.num_face_vertices[f]);

				// Loop over vertices in face
				for (size_t v = 0; v < fv; v++)
				{
					vertex = {};

					// Store index into attrib vertices vector
					tinyobj::index_t idx = shape.mesh.indices[subMeshInfo.indexCount + v];

					vertex.pos.x = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 0];
					vertex.pos.y = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 1];
					vertex.pos.z = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 2];

					// Check if 'normal_index' is zero or positive. negative = no normal data
					if (idx.normal_index >= 0)
					{
						vertex.normal.x = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 0];
						vertex.normal.y = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 1];
						vertex.normal.z = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 2];
					}

					// Check if `texcoord_index` is zero or positive. negative = no texcoord data
					if (idx.texcoord_index >= 0)
					{
						vertex.uvX = attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 0];
						vertex.uvY = attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 1];
					}

					// Add vertex to vertices vector if it does not already exist, else just add a new index to indices vector
					if (auto it = vertexIndices.find(vertex); it != vertexIndices.end())
					{
						indices.push_back(it->second);
					}
					else
					{
						vertices.push_back(vertex);
						indices.push_back(subMeshInfo.vertexCount);
						vertexIndices[vertex] = subMeshInfo.vertexCount;

						subMeshInfo.vertexCount++;
					}
				}

				subMeshInfo.indexCount += fv;
			}

			// Fill out SubMeshInfo struct
			subMeshInfo.vertexByteSizeTotal = subMeshInfo.vertexCount * sizeof(rendering::VertexPNTV32);
			subMeshInfo.indexByteSizeTotal = subMeshInfo.indexCount * sizeof(uint32_t);
			subMeshInfo.subMeshIdx = s;

			meshAssetInfo.subMeshInfos.push_back(subMeshInfo);

			vertexIndices.clear();

			s++;
		}

		GenerateTangents(vertices, indices);

		// Create & save asset
		meshAssetInfo.compressionMode = assets::CompressionMode::LZ4;
		meshAssetInfo.originalFile = modelPath.string();
		meshAssetInfo.vertexFormat = rendering::VertexFormat::PNTV32;

		for (auto& subMeshInfo : meshAssetInfo.subMeshInfos)
		{
			meshAssetInfo.vertexCountTotal += subMeshInfo.vertexCount;
			meshAssetInfo.indexCountTotal += subMeshInfo.indexCount;
			meshAssetInfo.vertexByteSizeTotal += subMeshInfo.vertexByteSizeTotal;
			meshAssetInfo.indexByteSizeTotal += subMeshInfo.indexByteSizeTotal;
		}

		fs::path assetPath = assetSubdirectory / (modelPath.stem().string() + ".pstaticmesh");

		if (auto asset = assets::AssetRegistry::Get()->AddAsset<assets::StaticMeshAsset>(assetPath); !asset->Save(meshAssetInfo, vertices.data(), indices.data()))
			return false;

		assets::AssetRegistry::Get()->SaveAssetCache();

		vertices.clear();
		indices.clear();

		return true;
	}

	// GLTF

	bool LoadGltfModel(const fs::path& modelPath, tinygltf::Model& model)
	{
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		std::cout << "Loading glTF: " << modelPath << std::endl;

		bool ret = false;
		if (modelPath.extension() == ".gltf")
		{
			ret = loader.LoadASCIIFromFile(&model, &err, &warn, modelPath.string());
		}
		else if (modelPath.extension() == ".glb")
		{
			ret = loader.LoadBinaryFromFile(&model, &err, &warn, modelPath.string());
		}

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

	bool ImportGltfModel(const fs::path& modelPath, const tinygltf::Model& model, fs::path assetSubdirectory)
	{
		// Import each mesh in file
		for (const auto& mesh : model.meshes)
		{
			if (mesh.primitives.empty())
				continue;

			assets::MeshAssetInfo meshAssetInfo;
			meshAssetInfo.subMeshInfos.reserve(mesh.primitives.size());

			size_t indexCountTotal = 0;
			for (const auto& primitive : mesh.primitives)
			{
				const auto& indexAccessor = model.accessors[primitive.indices];

				indexCountTotal += indexAccessor.count;
			}

			std::vector<rendering::VertexPNTV32> vertices;
			std::vector<uint32_t> indices;

			vertices.reserve(indexCountTotal);
			indices.reserve(indexCountTotal);

			int p = 0;
			for (const auto& primitive : mesh.primitives)
			{
				assets::SubMeshInfo subMeshInfo;
				subMeshInfo.vertexOffset = vertices.size();
				subMeshInfo.indexOffset = indices.size();

				std::vector<uint32_t> pIndices;

				// Load Indices
				{
					const auto& indexAccessor = model.accessors[primitive.indices];

					std::vector<unsigned short> indexShort;

					if (indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT && indexAccessor.type == TINYGLTF_TYPE_SCALAR)
					{
						const auto& bufferView = model.bufferViews[indexAccessor.bufferView];
						subMeshInfo.indexCount = indexAccessor.count;

						indexShort.resize(indexAccessor.count);

						copy_gltf_buffer_data(bufferView, model.buffers[bufferView.buffer], 2, indexShort.data());
					}

					// Copy indices into vector
					for (const auto& index : indexShort)
					{
						pIndices.push_back(index);
					}
				}

				std::vector<rendering::VertexPNTV32> pVertices;
				std::vector<Vector3f> vertexPos;
				std::vector<Vector3f> vertexNormal;
				std::vector<Vector3f> vertexTangent;
				std::vector<Vector2f> vertexUV;

				pVertices.reserve(subMeshInfo.indexCount);
				vertexPos.reserve(subMeshInfo.indexCount);
				vertexNormal.reserve(subMeshInfo.indexCount);
				vertexNormal.reserve(subMeshInfo.indexCount);
				vertexUV.reserve(subMeshInfo.indexCount);

				// Load Vertices
				for (const auto& attribute : primitive.attributes)
				{
					if (attribute.first == "POSITION")
					{
						const auto& accessor = model.accessors[attribute.second];

						if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC3)
						{
							const auto& buffer_view = model.bufferViews[accessor.bufferView];
							subMeshInfo.vertexCount = accessor.count;

							vertexPos.resize(subMeshInfo.vertexCount);

							copy_gltf_buffer_data(buffer_view, model.buffers[buffer_view.buffer], 12, vertexPos.data());
						}

						continue;
					}

					if (attribute.first == "NORMAL")
					{
						const auto& accessor = model.accessors[attribute.second];

						if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC3)
						{
							const auto& buffer_view = model.bufferViews[accessor.bufferView];
							subMeshInfo.vertexCount = accessor.count;

							vertexNormal.resize(subMeshInfo.vertexCount);

							copy_gltf_buffer_data(buffer_view, model.buffers[buffer_view.buffer], 12, vertexNormal.data());
						}

						continue;
					}

					if (attribute.first == "TANGENT")
					{
						const auto& accessor = model.accessors[attribute.second];

						if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC4)
						{
							const auto& buffer_view = model.bufferViews[accessor.bufferView];
							subMeshInfo.vertexCount = accessor.count;

							vertexTangent.resize(subMeshInfo.vertexCount);

							copy_gltf_buffer_data(buffer_view, model.buffers[buffer_view.buffer], 12, vertexTangent.data());
						}

						continue;
					}

					if (attribute.first == "TEXCOORD_0")
					{
						const auto& accessor = model.accessors[attribute.second];

						if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC2)
						{
							const auto& buffer_view = model.bufferViews[accessor.bufferView];
							subMeshInfo.vertexCount = accessor.count;

							vertexUV.resize(subMeshInfo.vertexCount);

							copy_gltf_buffer_data(buffer_view, model.buffers[buffer_view.buffer], 8, vertexUV.data());
						}
					}
				}

				// Copy data into vertices vector
				for (int i = 0; i < subMeshInfo.vertexCount; i++)
				{
					rendering::VertexPNTV32 vertex;

					if (!vertexPos.empty())
						vertex.pos = vertexPos[i];

					if (!vertexNormal.empty())
						vertex.normal = vertexNormal[i];

					if (!vertexTangent.empty())
						vertex.tangent = vertexTangent[i];

					if (!vertexUV.empty())
					{
						vertex.uvX = vertexUV[i].x;
						vertex.uvY = vertexUV[i].y;
					}

					pVertices.push_back(vertex);
				}

				vertexPos.clear();
				vertexNormal.clear();
				vertexTangent.clear();
				vertexUV.clear();

				// Generate tangents if none were loaded from file
				if (vertexTangent.size() < pVertices.size())
					GenerateTangents(pVertices, pIndices);

				subMeshInfo.vertexByteSizeTotal = pVertices.size() * sizeof(rendering::VertexPNTV32);
				subMeshInfo.indexByteSizeTotal = pIndices.size() * sizeof(uint32_t);
				subMeshInfo.subMeshIdx = p;

				meshAssetInfo.subMeshInfos.push_back(subMeshInfo);

				// Copy primitive vertices/indices to main vectors and clear
				for (const auto& v : pVertices)
				{
					vertices.push_back(v);
				}

				for (const auto& i : pIndices)
				{
					indices.push_back(i);
				}

				pVertices.clear();
				pIndices.clear();

				p++;
			}

			meshAssetInfo.compressionMode = assets::CompressionMode::LZ4;
			meshAssetInfo.originalFile = modelPath.string();
			meshAssetInfo.vertexFormat = rendering::VertexFormat::PNTV32;

			for (auto& subMeshInfo : meshAssetInfo.subMeshInfos)
			{
				meshAssetInfo.vertexCountTotal += subMeshInfo.vertexCount;
				meshAssetInfo.indexCountTotal += subMeshInfo.indexCount;
				meshAssetInfo.vertexByteSizeTotal += subMeshInfo.vertexByteSizeTotal;
				meshAssetInfo.indexByteSizeTotal += subMeshInfo.indexByteSizeTotal;
			}

			fs::path assetPath = assetSubdirectory / (modelPath.stem().string() + "_" + mesh.name + ".pstaticmesh");

			auto asset = assets::AssetRegistry::Get()->AddAsset<assets::StaticMeshAsset>(assetPath);

			if (!asset->Save(meshAssetInfo, vertices.data(), indices.data()))
				return false;

			assets::AssetRegistry::Get()->SaveAssetCache();

			vertices.clear();
			indices.clear();
		}

		// Import Textures
		/*for (const auto& image : model.images)
		{
			fs::path texturePath = modelPath.parent_path() / image.uri;

			if (!loadAndImportTexture(texturePath, assetSubdirectory))
				return false;
		}*/

		return true;
	}

	bool LoadAndImportGltfModel(const fs::path& modelPath, fs::path assetSubdirectory)
	{
		tinygltf::Model model;

		if (!LoadGltfModel(modelPath, model))
			return false;

		return ImportGltfModel(modelPath, model, assetSubdirectory);
	}

	//////////////////////
	// Texture Importers
	//////////////////////

#ifdef PFN_PLATFORM_WIN32
	const static std::unordered_map<assets::TextureFormat, CMP_FORMAT> gTexPuffinToCMPFormat =
	{
		{ assets::TextureFormat::R8, CMP_FORMAT_R_8 },
		{ assets::TextureFormat::RG8, CMP_FORMAT_RG_8 },
		{ assets::TextureFormat::RGB8, CMP_FORMAT_RGB_888 },
		{ assets::TextureFormat::RGBA8, CMP_FORMAT_RGBA_8888 },
		{ assets::TextureFormat::BC4, CMP_FORMAT_BC4 },
		{ assets::TextureFormat::BC5, CMP_FORMAT_BC5 },
		{ assets::TextureFormat::BC6H, CMP_FORMAT_BC6H },
		{ assets::TextureFormat::BC7, CMP_FORMAT_BC7 }
	};

	static bool ConvertTextureToBC(uint32_t width, uint32_t height, void* srcData, assets::TextureFormat srcFormat, uint32_t srcSize, std::vector<char>& dstData, assets::TextureFormat dstFormat, uint32_t& dstSize)
	{
		// Setup source texture
		CMP_Texture srcTexture;
		srcTexture.dwWidth = width;
		srcTexture.dwHeight = height;
		srcTexture.dwPitch = 0;
		srcTexture.dwSize = sizeof(srcTexture);
		srcTexture.format = gTexPuffinToCMPFormat.at(srcFormat);
		srcTexture.dwDataSize = srcSize;
		srcTexture.pData = static_cast<CMP_BYTE*>(srcData);

		// Setup destination
		CMP_Texture dstTexture;
		dstTexture.dwWidth = width;
		dstTexture.dwHeight = height;
		dstTexture.dwPitch = 0;
		dstTexture.dwSize = sizeof(srcTexture);
		dstTexture.format = gTexPuffinToCMPFormat.at(dstFormat);

		dstSize = CMP_CalculateBufferSize(&dstTexture);
		dstData.resize(dstSize);

		dstTexture.dwDataSize = dstSize;
		dstTexture.pData = reinterpret_cast<CMP_BYTE*>(dstData.data());

		// Setup compression options
		CMP_CompressOptions options = { 0 };
		options.dwSize = sizeof(options);

		// Compress texture
		CMP_ERROR cmpStatus;
		cmpStatus = CMP_ConvertTexture(&srcTexture, &dstTexture, &options, nullptr);
		if (cmpStatus != CMP_OK)
		{
			return false;
		}

		return true;
	}
#endif

	bool LoadAndImportTexture(fs::path texturePath, fs::path assetSubdirectory, bool useBCFormat)
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
		fs::path assetPath = assetSubdirectory / (texturePath.stem().string() + ".ptexture");

		assets::TextureInfo info;
		info.compressionMode = assets::CompressionMode::LZ4;
		info.originalFile = texturePath.string();
		info.textureHeight = static_cast<uint32_t>(texHeight);
		info.textureWidth = static_cast<uint32_t>(texWidth);
		info.textureChannels = static_cast<uint8_t>(desiredChannels);
		info.originalSize = info.textureHeight * info.textureWidth * info.textureChannels;

		const assets::TextureFormat rgbFormat = assets::gTexChannelsToRGBAFormat.at(info.textureChannels);
		const assets::TextureFormat bcFormat = assets::gTexChannelsToBCFormat.at(info.textureChannels);

		const auto asset = assets::AssetRegistry::Get()->AddAsset<assets::TextureAsset>(assetPath);

		bool ret;

		// Save asset
#ifdef PFN_PLATFORM_WIN32
		if (useBCFormat)
		{
			std::vector<char> compressedPixels;

			if (!ConvertTextureToBC(info.textureWidth, info.textureHeight, pixelPtr, rgbFormat, info.originalSize, compressedPixels, bcFormat, info.originalSize))
			{
				std::cout << "Failed to convert texture to bc format " << texturePath.string() << std::endl;
				return false;
			}

			info.textureFormat = bcFormat;
			ret = asset->Save(info, compressedPixels.data());

			compressedPixels.clear();
		}
		else
#endif
		{
			info.textureFormat = rgbFormat;
			ret = asset->Save(info, pixelPtr);
		}

		// Free Loaded Data, as pixels are now in staging buffer
		stbi_image_free(pixels);

		assets::AssetRegistry::Get()->SaveAssetCache();

		return ret;
	}
}
