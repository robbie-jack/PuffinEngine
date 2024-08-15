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
	void generate_tangents(std::vector<rendering::VertexPNTV32> vertices, std::vector<uint32_t> indices)
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
		const size_t byte_offset = bufferView.byteOffset;
		const size_t byte_length = bufferView.byteLength;
		const size_t byte_stride = bufferView.byteStride;

		const auto dst_char = static_cast<char*>(dst);

		// Data is packed together, copy with a single command
		if (byte_stride == 0 || byte_stride == expectedStride)
		{
			memcpy(dst_char, buffer.data.data() + byte_offset, byte_length);
		}
		// Byte data is interleaved, copy each attribute individually
		else
		{
			int i = 0;
			for (size_t offset = byte_offset; offset < byte_offset + byte_length; offset += byte_stride)
			{
				memcpy(dst_char + (i * byte_stride), buffer.data.data() + offset, expectedStride);

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
			return load_and_import_gltf_model(modelPath, assetSubdirectory);

		if (modelPath.extension() == ".obj")
			return load_and_import_obj_model(modelPath, assetSubdirectory);

		std::cout << "Failed to load mesh, unsupported format" << std::endl;

		return false;
	}

	// OBJ

	bool load_and_import_obj_model(const fs::path& modelPath, fs::path assetSubdirectory)
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

		assets::MeshAssetInfo mesh_asset_info;

		mesh_asset_info.sub_mesh_infos.reserve(shapes.size());

		size_t index_count_total = 0;
		for (const auto& shape : shapes)
		{
			index_count_total += shape.mesh.indices.size();
		}

		vertices.reserve(index_count_total);
		indices.reserve(index_count_total);

		// Loop over shapes
		int s = 0;
		for (const auto& shape : shapes)
		{
			rendering::VertexPNTV32 vertex;

			assets::SubMeshInfo sub_mesh_info;
			sub_mesh_info.vertex_offset = vertices.size();
			sub_mesh_info.index_offset = indices.size();

			std::unordered_map<rendering::VertexPNTV32, uint32_t> vertex_indices;

			// Loop over faces (polygon) to get indices
			for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
			{
				auto fv = static_cast<size_t>(shape.mesh.num_face_vertices[f]);

				// Loop over vertices in face
				for (size_t v = 0; v < fv; v++)
				{
					vertex = {};

					// Store index into attrib vertices vector
					tinyobj::index_t idx = shape.mesh.indices[sub_mesh_info.index_count + v];

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
					if (auto it = vertex_indices.find(vertex); it != vertex_indices.end())
					{
						indices.push_back(it->second);
					}
					else
					{
						vertices.push_back(vertex);
						indices.push_back(sub_mesh_info.vertex_count);
						vertex_indices[vertex] = sub_mesh_info.vertex_count;

						sub_mesh_info.vertex_count++;
					}
				}

				sub_mesh_info.index_count += fv;
			}

			// Fill out SubMeshInfo struct
			sub_mesh_info.vertex_byte_size_total = sub_mesh_info.vertex_count * sizeof(rendering::VertexPNTV32);
			sub_mesh_info.index_byte_size_total = sub_mesh_info.index_count * sizeof(uint32_t);
			sub_mesh_info.sub_mesh_idx = s;

			mesh_asset_info.sub_mesh_infos.push_back(sub_mesh_info);

			vertex_indices.clear();

			s++;
		}

		generate_tangents(vertices, indices);

		// Create & save asset
		mesh_asset_info.compressionMode = assets::CompressionMode::LZ4;
		mesh_asset_info.originalFile = modelPath.string();
		mesh_asset_info.vertex_format = rendering::VertexFormat::PNTV32;

		for (auto& sub_mesh_info : mesh_asset_info.sub_mesh_infos)
		{
			mesh_asset_info.vertex_count_total += sub_mesh_info.vertex_count;
			mesh_asset_info.index_count_total += sub_mesh_info.index_count;
			mesh_asset_info.vertex_byte_size_total += sub_mesh_info.vertex_byte_size_total;
			mesh_asset_info.index_byte_size_total += sub_mesh_info.index_byte_size_total;
		}

		fs::path asset_path = assetSubdirectory / (modelPath.stem().string() + ".pstaticmesh");

		auto asset = assets::AssetRegistry::get()->add_asset<assets::StaticMeshAsset>(asset_path);

		if (!asset->save(mesh_asset_info, vertices.data(), indices.data()))
			return false;

		assets::AssetRegistry::get()->save_asset_cache();

		vertices.clear();
		indices.clear();

		return true;
	}

	// GLTF

	bool load_gltf_model(const fs::path& modelPath, tinygltf::Model& model)
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

	bool import_gltf_model(const fs::path& modelPath, const tinygltf::Model& model, fs::path assetSubdirectory)
	{
		// Import each mesh in file
		for (const auto& mesh : model.meshes)
		{
			if (mesh.primitives.empty())
				continue;

			assets::MeshAssetInfo mesh_asset_info;
			mesh_asset_info.sub_mesh_infos.reserve(mesh.primitives.size());

			size_t index_count_total = 0;
			for (const auto& primitive : mesh.primitives)
			{
				const auto& indexAccessor = model.accessors[primitive.indices];

				index_count_total += indexAccessor.count;
			}

			std::vector<rendering::VertexPNTV32> vertices;
			std::vector<uint32_t> indices;

			vertices.reserve(index_count_total);
			indices.reserve(index_count_total);

			int p = 0;
			for (const auto& primitive : mesh.primitives)
			{
				assets::SubMeshInfo sub_mesh_info;
				sub_mesh_info.vertex_offset = vertices.size();
				sub_mesh_info.index_offset = indices.size();

				std::vector<uint32_t> p_indices;

				// Load Indices
				{
					const auto& indexAccessor = model.accessors[primitive.indices];

					std::vector<unsigned short> indexShort;

					if (indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT && indexAccessor.type == TINYGLTF_TYPE_SCALAR)
					{
						const auto& bufferView = model.bufferViews[indexAccessor.bufferView];
						sub_mesh_info.index_count = indexAccessor.count;

						indexShort.resize(indexAccessor.count);

						copy_gltf_buffer_data(bufferView, model.buffers[bufferView.buffer], 2, indexShort.data());
					}

					// Copy indices into vector
					for (const auto& index : indexShort)
					{
						p_indices.push_back(index);
					}
				}

				std::vector<rendering::VertexPNTV32> p_vertices;
				std::vector<Vector3f> vertex_pos;
				std::vector<Vector3f> vertex_normal;
				std::vector<Vector3f> vertex_tangent;
				std::vector<Vector2f> vertex_uv;

				p_vertices.reserve(sub_mesh_info.index_count);
				vertex_pos.reserve(sub_mesh_info.index_count);
				vertex_normal.reserve(sub_mesh_info.index_count);
				vertex_normal.reserve(sub_mesh_info.index_count);
				vertex_uv.reserve(sub_mesh_info.index_count);

				// Load Vertices
				for (const auto& attribute : primitive.attributes)
				{
					if (attribute.first == "POSITION")
					{
						const auto& accessor = model.accessors[attribute.second];

						if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC3)
						{
							const auto& buffer_view = model.bufferViews[accessor.bufferView];
							sub_mesh_info.vertex_count = accessor.count;

							vertex_pos.resize(sub_mesh_info.vertex_count);

							copy_gltf_buffer_data(buffer_view, model.buffers[buffer_view.buffer], 12, vertex_pos.data());
						}

						continue;
					}

					if (attribute.first == "NORMAL")
					{
						const auto& accessor = model.accessors[attribute.second];

						if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC3)
						{
							const auto& buffer_view = model.bufferViews[accessor.bufferView];
							sub_mesh_info.vertex_count = accessor.count;

							vertex_normal.resize(sub_mesh_info.vertex_count);

							copy_gltf_buffer_data(buffer_view, model.buffers[buffer_view.buffer], 12, vertex_normal.data());
						}

						continue;
					}

					if (attribute.first == "TANGENT")
					{
						const auto& accessor = model.accessors[attribute.second];

						if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC4)
						{
							const auto& buffer_view = model.bufferViews[accessor.bufferView];
							sub_mesh_info.vertex_count = accessor.count;

							vertex_tangent.resize(sub_mesh_info.vertex_count);

							copy_gltf_buffer_data(buffer_view, model.buffers[buffer_view.buffer], 12, vertex_tangent.data());
						}

						continue;
					}

					if (attribute.first == "TEXCOORD_0")
					{
						const auto& accessor = model.accessors[attribute.second];

						if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC2)
						{
							const auto& buffer_view = model.bufferViews[accessor.bufferView];
							sub_mesh_info.vertex_count = accessor.count;

							vertex_uv.resize(sub_mesh_info.vertex_count);

							copy_gltf_buffer_data(buffer_view, model.buffers[buffer_view.buffer], 8, vertex_uv.data());
						}
					}
				}

				// Copy data into vertices vector
				for (int i = 0; i < sub_mesh_info.vertex_count; i++)
				{
					rendering::VertexPNTV32 vertex;

					if (!vertex_pos.empty())
						vertex.pos = vertex_pos[i];

					if (!vertex_normal.empty())
						vertex.normal = vertex_normal[i];

					if (!vertex_tangent.empty())
						vertex.tangent = vertex_tangent[i];

					if (!vertex_uv.empty())
					{
						vertex.uvX = vertex_uv[i].x;
						vertex.uvY = vertex_uv[i].y;
					}

					p_vertices.push_back(vertex);
				}

				vertex_pos.clear();
				vertex_normal.clear();
				vertex_tangent.clear();
				vertex_uv.clear();

				// Generate tangents if none were loaded from file
				if (vertex_tangent.size() < p_vertices.size())
					generate_tangents(p_vertices, p_indices);

				sub_mesh_info.vertex_byte_size_total = p_vertices.size() * sizeof(rendering::VertexPNTV32);
				sub_mesh_info.index_byte_size_total = p_indices.size() * sizeof(uint32_t);
				sub_mesh_info.sub_mesh_idx = p;

				mesh_asset_info.sub_mesh_infos.push_back(sub_mesh_info);

				// Copy primitive vertices/indices to main vectors and clear
				for (const auto& v : p_vertices)
				{
					vertices.push_back(v);
				}

				for (const auto& i : p_indices)
				{
					indices.push_back(i);
				}

				p_vertices.clear();
				p_indices.clear();

				p++;
			}

			mesh_asset_info.compressionMode = assets::CompressionMode::LZ4;
			mesh_asset_info.originalFile = modelPath.string();
			mesh_asset_info.vertex_format = rendering::VertexFormat::PNTV32;

			for (auto& sub_mesh_info : mesh_asset_info.sub_mesh_infos)
			{
				mesh_asset_info.vertex_count_total += sub_mesh_info.vertex_count;
				mesh_asset_info.index_count_total += sub_mesh_info.index_count;
				mesh_asset_info.vertex_byte_size_total += sub_mesh_info.vertex_byte_size_total;
				mesh_asset_info.index_byte_size_total += sub_mesh_info.index_byte_size_total;
			}

			fs::path asset_path = assetSubdirectory / (modelPath.stem().string() + "_" + mesh.name + ".pstaticmesh");

			auto asset = assets::AssetRegistry::get()->add_asset<assets::StaticMeshAsset>(asset_path);

			if (!asset->save(mesh_asset_info, vertices.data(), indices.data()))
				return false;

			assets::AssetRegistry::get()->save_asset_cache();

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

	bool load_and_import_gltf_model(const fs::path& modelPath, fs::path assetSubdirectory)
	{
		tinygltf::Model model;

		if (!load_gltf_model(modelPath, model))
			return false;

		return import_gltf_model(modelPath, model, assetSubdirectory);
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

	bool convertTextureToBC(uint32_t width, uint32_t height, void* srcData, assets::TextureFormat srcFormat, uint32_t srcSize, std::vector<char>& dstData, assets::TextureFormat dstFormat, uint32_t& dstSize)
	{
		// Setup source texture
		CMP_Texture src_texture;
		src_texture.dwWidth = width;
		src_texture.dwHeight = height;
		src_texture.dwPitch = 0;
		src_texture.dwSize = sizeof(src_texture);
		src_texture.format = gTexPuffinToCMPFormat.at(srcFormat);
		src_texture.dwDataSize = srcSize;
		src_texture.pData = static_cast<CMP_BYTE*>(srcData);

		// Setup destination
		CMP_Texture dst_texture;
		dst_texture.dwWidth = width;
		dst_texture.dwHeight = height;
		dst_texture.dwPitch = 0;
		dst_texture.dwSize = sizeof(src_texture);
		dst_texture.format = gTexPuffinToCMPFormat.at(dstFormat);

		dstSize = CMP_CalculateBufferSize(&dst_texture);
		dstData.resize(dstSize);

		dst_texture.dwDataSize = dstSize;
		dst_texture.pData = reinterpret_cast<CMP_BYTE*>(dstData.data());

		// Setup compression options
		CMP_CompressOptions options = { 0 };
		options.dwSize = sizeof(options);

		// Compress texture
		CMP_ERROR cmp_status;
		cmp_status = CMP_ConvertTexture(&src_texture, &dst_texture, &options, nullptr);
		if (cmp_status != CMP_OK)
		{
			return false;
		}

		return true;
	}
#endif

	bool LoadAndImportTexture(fs::path texturePath, fs::path assetSubdirectory, bool useBCFormat)
	{
		int tex_width, tex_height, tex_channels;

		// Load info about texture without loading full file
		if (const int ok = stbi_info(texturePath.string().c_str(), &tex_width, &tex_height, &tex_channels); ok == 0)
		{
			std::cout << "Failed to load texture file " << texturePath.string() << std::endl;
			return false;
		}

		// Figure out how many channels loaded pixel data should have (rgb data gets an extra channel since it's stored the same as rgba data)
		int desired_channels;
		if (tex_channels == 3)
		{
			desired_channels = 4;
		}
		else
		{
			desired_channels = tex_channels;
		}

		// Load pixel data
		stbi_uc* pixels = stbi_load(texturePath.string().c_str(), &tex_width, &tex_height, &tex_channels, desired_channels);

		if (!pixels) {
			std::cout << "Failed to load texture file " << texturePath.string() << std::endl;
			return false;
		}

		void* pixel_ptr = pixels;

		// Instantiate new Texture Asset to store loaded Pixel data
		fs::path assetPath = assetSubdirectory / (texturePath.stem().string() + ".ptexture");

		assets::TextureInfo info;
		info.compressionMode = assets::CompressionMode::LZ4;
		info.originalFile = texturePath.string();
		info.textureHeight = static_cast<uint32_t>(tex_height);
		info.textureWidth = static_cast<uint32_t>(tex_width);
		info.textureChannels = static_cast<uint8_t>(desired_channels);
		info.originalSize = info.textureHeight * info.textureWidth * info.textureChannels;

		const assets::TextureFormat rgb_format = assets::gTexChannelsToRGBAFormat.at(info.textureChannels);
		const assets::TextureFormat bc_format = assets::gTexChannelsToBCFormat.at(info.textureChannels);

		const auto asset = assets::AssetRegistry::get()->add_asset<assets::TextureAsset>(assetPath);

		bool ret;

		// Save asset
#ifdef PFN_PLATFORM_WIN32
		if (useBCFormat)
		{
			std::vector<char> compressed_pixels;

			if (!convertTextureToBC(info.textureWidth, info.textureHeight, pixel_ptr, rgb_format, info.originalSize, compressed_pixels, bc_format, info.originalSize))
			{
				std::cout << "Failed to convert texture to bc format " << texturePath.string() << std::endl;
				return false;
			}

			info.textureFormat = bc_format;
			ret = asset->save(info, compressed_pixels.data());

			compressed_pixels.clear();
		}
		else
#endif
		{
			info.textureFormat = rgb_format;
			ret = asset->save(info, pixel_ptr);
		}

		// Free Loaded Data, as pixels are now in staging buffer
		stbi_image_free(pixels);

		assets::AssetRegistry::get()->save_asset_cache();

		return ret;
	}
}
