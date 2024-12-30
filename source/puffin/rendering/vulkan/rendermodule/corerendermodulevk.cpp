#include "puffin/rendering/vulkan/rendermodule/corerendermodulevk.h"

#include "vk_mem_alloc.hpp"
#include "puffin/assets/assetregistry.h"
#include "puffin/assets/staticmeshasset.h"

#include "puffin/rendering/renderglobals.h"
#include "puffin/rendering/vulkan/rendersubsystemvk.h"
#include "puffin/rendering/vulkan/resourcedescvk.h"
#include "puffin/rendering/vulkan/resourcemanagervk.h"
#include "puffin/components/transformcomponent2d.h"
#include "puffin/components/transformcomponent3d.h"
#include "puffin/components/rendering/3d/staticmeshcomponent3d.h"
#include "puffin/rendering/vulkan/unifiedgeometrybuffervk.h"
#include "puffin/rendering/vulkan/materialregistryvk.h"
#include "puffin/rendering/vulkan/texturemanagervk.h"

namespace puffin::rendering
{
	CoreRenderModuleVK::CoreRenderModuleVK(std::shared_ptr<core::Engine> engine, RenderSubsystemVK* renderSubsystem)
		: RenderModuleVK(engine, renderSubsystem)
	{
	}

	void CoreRenderModuleVK::RegisterModules()
	{
		RenderModuleVK::RegisterModules();
	}

	void CoreRenderModuleVK::Initialize()
	{
		RenderModuleVK::Initialize();

		BindCallbacks();
		InitBuffers();
		InitSamplers();
		InitDescriptorLayouts();

		mUpdateRenderables = true;
		mInitialized = true;
	}

	void CoreRenderModuleVK::Deinitialize()
	{
		mRenderSubsystem->GetResourceManager()->DestroyResource(mIndirectDrawBufferID);
		mIndirectDrawBufferID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mCameraBufferID);
		mCameraBufferID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mPointLightBufferID);
		mPointLightBufferID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mSpotLightBufferID);
		mSpotLightBufferID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mDirLightBufferID);
		mDirLightBufferID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mObjectBufferID);
		mObjectBufferID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mMaterialInstanceBufferID);
		mMaterialInstanceBufferID = gInvalidID;

		mRenderSubsystem->GetDevice().destroySampler(mTextureSampler);

		mRenderSubsystem->GetResourceManager()->DestroyResource(mObjectDescriptorLayoutID);
		mObjectDescriptorLayoutID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mLightDescriptorLayoutID);
		mLightDescriptorLayoutID = gInvalidID;

		mRenderSubsystem->GetResourceManager()->DestroyResource(mMatDescriptorLayoutID);
		mMatDescriptorLayoutID = gInvalidID;

		mInitialized = false;

		RenderModuleVK::Deinitialize();
	}

	void CoreRenderModuleVK::PostInitialize()
	{
		RenderModuleVK::PostInitialize();

		InitDescriptorSets();
	}

	void CoreRenderModuleVK::UpdateResources(ResourceManagerVK* resourceManager)
	{
		RenderModuleVK::UpdateResources(resourceManager);

		// Load Meshes
		{
			for (const auto meshID : mMeshesToLoad)
			{
				if (const auto staticMesh = assets::AssetRegistry::Get()->GetAsset<assets::StaticMeshAsset>(meshID))
				{
					mRenderSubsystem->GetUnifiedGeometryBuffer()->AddStaticMesh(staticMesh);
				}
			}

			mMeshesToLoad.clear();
		}
	}

	void CoreRenderModuleVK::UpdateGraph(RenderGraphVK& renderGraph)
	{
		RenderModuleVK::UpdateGraph(renderGraph);
	}

	void CoreRenderModuleVK::PreRender(double deltaTime)
	{
		RenderModuleVK::PreRender(deltaTime);

		// Upload material data to gpu when updated
		{
			if (mRenderSubsystem->GetMaterialRegistry()->MaterialDataNeedsUploaded())
			{
				for (uint32_t i = 0; i < gBufferedFrameCount; i++)
				{
					mFrameRenderData[i].copyMaterialDataToGPU = true;
				}
			}
		}

		// Update texture data and descriptor if needed
		{
			const auto& textureManager = mRenderSubsystem->GetTextureManager();
			
			if (textureManager->TextureDescriptorNeedsUpdated() == true)
			{
				mTexData.Clear();

				for (const auto& [id, texture] : textureManager->GetLoadedTextures())
				{
					TextureDataVK texData;
					texData.id = id;
					texData.sampler = mTextureSampler;
					
					mTexData.Emplace(id, texData);
				}
				
				for (int i = 0; i < gBufferedFrameCount; i++)
				{
					mFrameRenderData[i].textureDescriptorNeedsUpdated = true;
				}
			}
		}
		
		ProcessComponents();
		UpdateTextureDescriptors();
		PrepareSceneData();
		BuildIndirectCommands();
	}

	void CoreRenderModuleVK::OnUpdateMesh(entt::registry& registry, entt::entity entity)
	{
		const auto mesh = registry.get<StaticMeshComponent3D>(entity);

		if (mesh.meshID == gInvalidID || mesh.materialID == gInvalidID)
		{
			return;
		}

		mMeshesToLoad.insert(mesh.meshID);
		mRenderSubsystem->GetMaterialRegistry()->AddMaterialInstanceToLoad(mesh.materialID);

		AddRenderable(registry, entity);
	}

	void CoreRenderModuleVK::OnUpdateTransform(entt::registry& registry, entt::entity entity)
	{
		AddRenderable(registry, entity);
	}

	void CoreRenderModuleVK::OnDestroyMeshOrTransform(entt::registry& registry, entt::entity entity)
	{
		mUpdateRenderables = true;
	}

	void CoreRenderModuleVK::AddRenderable(entt::registry& registry, entt::entity entity)
	{
		if (registry.any_of<TransformComponent2D, TransformComponent3D>(entity) && registry.any_of<StaticMeshComponent3D>(entity))
		{
			const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
			const auto id = enttSubsystem->GetID(entity);
			const auto mesh = registry.get<StaticMeshComponent3D>(entity);

			if (mesh.meshID == gInvalidID || mesh.materialID == gInvalidID)
			{
				return;
			}

			mObjectsToUpdate.emplace(id);

			mUpdateRenderables = true;
		}
	}

	void CoreRenderModuleVK::BindCallbacks()
	{
		const auto& enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		
		// Bind callbacks
		const auto registry = enttSubsystem->GetRegistry();

		registry->on_construct<StaticMeshComponent3D>().connect<&CoreRenderModuleVK::OnUpdateMesh>(this);
		registry->on_update<StaticMeshComponent3D>().connect<&CoreRenderModuleVK::OnUpdateMesh>(this);
		registry->on_destroy<StaticMeshComponent3D>().connect<&CoreRenderModuleVK::OnDestroyMeshOrTransform>(this);

		registry->on_construct<TransformComponent2D>().connect<&CoreRenderModuleVK::OnUpdateTransform>(this);
		registry->on_update<TransformComponent2D>().connect<&CoreRenderModuleVK::OnUpdateTransform>(this);
		registry->on_destroy<TransformComponent2D>().connect<&CoreRenderModuleVK::OnDestroyMeshOrTransform>(this);

		registry->on_construct<TransformComponent3D>().connect<&CoreRenderModuleVK::OnUpdateTransform>(this);
		registry->on_update<TransformComponent3D>().connect<&CoreRenderModuleVK::OnUpdateTransform>(this);
		registry->on_destroy<TransformComponent3D>().connect<&CoreRenderModuleVK::OnDestroyMeshOrTransform>(this);
	}

	void CoreRenderModuleVK::InitBuffers()
	{
		ResourceManagerVK* resourceManager = mRenderSubsystem->GetResourceManager();

		// Indirect Buffer
		BufferDescVK bufferDesc;
		bufferDesc.size = sizeof(vk::DrawIndexedIndirectCommand) * gMaxObjects;
		bufferDesc.usage = vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
			vk::BufferUsageFlagBits::eTransferDst;
		bufferDesc.allocFlags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped;
		mIndirectDrawBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "indirect_draw");

		// Camera Buffer
		bufferDesc.size = sizeof(GPUCameraData);
		bufferDesc.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
		mCameraBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "camera");

		// Light Buffers
		bufferDesc.size = sizeof(GPUPointLightData) * gMaxPointLights;
		bufferDesc.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
		mPointLightBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "point_lights");

		bufferDesc.size = sizeof(GPUSpotLightData) * gMaxSpotLights;
		mSpotLightBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "spot_lights");

		bufferDesc.size = sizeof(GPUDirLightData) * gMaxDirectionalLights;
		mDirLightBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "dir_lights");

		// Object Buffer
		bufferDesc.size = sizeof(GPUObjectData) * gMaxObjects;
		mObjectBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "objects");

		// Material Buffer
		bufferDesc.size = sizeof(GPUMaterialInstanceData) * gMaxMaterialInstances;
		mMaterialInstanceBufferID = resourceManager->CreateOrUpdateBuffer(bufferDesc, "material_instances");

	}

	void CoreRenderModuleVK::InitSamplers()
	{
		vk::SamplerCreateInfo textureSamplerInfo = {};
		textureSamplerInfo.anisotropyEnable = true;
		textureSamplerInfo.maxAnisotropy = 16.0f;

		mTextureSampler = mRenderSubsystem->GetDevice().createSampler(textureSamplerInfo);
	}

	void CoreRenderModuleVK::InitDescriptorLayouts()
	{
		ResourceManagerVK* resourceManager = mRenderSubsystem->GetResourceManager();
		
		DescriptorLayoutDescVK descriptorLayoutDesc;
		
		// Object Layout
		{
			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex });
			
			mObjectDescriptorLayoutID = resourceManager->CreateOrUpdateDescriptorLayout(descriptorLayoutDesc, "objects");
		}

		// Light Layout
		{
			descriptorLayoutDesc.bindings.clear();

			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });

			mLightDescriptorLayoutID = resourceManager->CreateOrUpdateDescriptorLayout(descriptorLayoutDesc, "lights");
		}

		// Material Layout
		{
			descriptorLayoutDesc.bindings.clear();

			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eCombinedImageSampler, gMaxMaterialInstances, vk::ShaderStageFlagBits::eFragment, 
				{ vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount } });

			mMatDescriptorLayoutID = resourceManager->CreateOrUpdateDescriptorLayout(descriptorLayoutDesc, "materials");
		}

		// Shadow Layout - PUFFIN_TODO - Dummy layout, will be removed once shadows are implemented in own render module
		{
			constexpr uint32_t imageCount = 128;
			
			descriptorLayoutDesc.bindings.clear();

			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment });
			descriptorLayoutDesc.bindings.push_back({ vk::DescriptorType::eCombinedImageSampler, imageCount, vk::ShaderStageFlagBits::eFragment,
				{ vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount } });

			mShadowDescriptorLayoutID = resourceManager->CreateOrUpdateDescriptorLayout(descriptorLayoutDesc, "shadows");
		}
	}

	void CoreRenderModuleVK::InitDescriptorSets()
	{
		ResourceManagerVK* resourceManager = mRenderSubsystem->GetResourceManager();
		util::DescriptorAllocator* descriptorAllocator = mRenderSubsystem->GetDescriptorAllocator();
		util::DescriptorLayoutCache* descriptorLayoutCache = mRenderSubsystem->GetDescriptorLayoutCache();
		
		DescriptorLayoutDescVK descriptorLayoutDesc;

		for (int i = 0; i < gBufferedFrameCount; ++i)
		{
			// Object Descriptor Set
			{
				vk::DescriptorBufferInfo objectBufferInfo =
				{
					resourceManager->GetBuffer(mObjectBufferID, i).buffer, 0, sizeof(GPUObjectData) * gMaxObjects,
				};

				util::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
				.BindBuffer(0, &objectBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
				.Build(mFrameRenderData[i].objectDescriptor, resourceManager->GetDescriptorLayout(mObjectDescriptorLayoutID));
			}

			// Light Descriptor Set
			{
				vk::DescriptorBufferInfo pointLightBufferInfo =
				{
					resourceManager->GetBuffer(mPointLightBufferID, i).buffer, 0, sizeof(GPUPointLightData) * gMaxPointLights,
				};

				vk::DescriptorBufferInfo spotLightBufferInfo =
				{
					resourceManager->GetBuffer(mSpotLightBufferID, i).buffer, 0, sizeof(GPUSpotLightData) * gMaxSpotLights,
				};

				vk::DescriptorBufferInfo dirLightBufferInfo =
				{
					resourceManager->GetBuffer(mDirLightBufferID, i).buffer, 0, sizeof(GPUDirLightData) * gMaxDirectionalLights,
				};

				util::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
				.BindBuffer(0, &pointLightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.BindBuffer(1, &spotLightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.BindBuffer(2, &dirLightBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.Build(mFrameRenderData[i].lightDescriptor, resourceManager->GetDescriptorLayout(mLightDescriptorLayoutID));
			}
			
			// Material Descriptor Set
			{
				constexpr uint32_t imageCount = 128;
				
				constexpr vk::DescriptorBindingFlags descriptorBindingFlags = { vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount };
				vk::DescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountAllocInfo =
				{ 1, &imageCount };

				vk::DescriptorBufferInfo materialBufferInfo = {
					resourceManager->GetBuffer(mMaterialInstanceBufferID, i).buffer, 0, sizeof(GPUMaterialInstanceData) * gMaxMaterialInstances
				};

				util::DescriptorBuilder::Begin(descriptorLayoutCache, descriptorAllocator)
				.BindBuffer(0, &materialBufferInfo, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
				.BindImagesWithoutWrite(1, imageCount, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, descriptorBindingFlags)
				.AddPNext(&variableDescriptorCountAllocInfo)
				.Build(mFrameRenderData[i].materialDescriptor, resourceManager->GetDescriptorLayout(mMatDescriptorLayoutID));
			}

			// Shadow Descriptor Set - PUFFIN_TODO - Dummy set, will be removed once shadows are implemented in own render module
			{
				constexpr uint32_t lightCount = gMaxLights;
			}
		}
	}

	void CoreRenderModuleVK::ProcessComponents()
	{
		const auto enttSubsystem = mEngine->GetSubsystem<ecs::EnTTSubsystem>();
		const auto registry = enttSubsystem->GetRegistry();

		const auto materialRegistry = mRenderSubsystem->GetMaterialRegistry();

		if (mUpdateRenderables)
		{
			const auto meshView2D = registry->view<const TransformComponent2D, const StaticMeshComponent3D>();
			const auto meshView3D = registry->view<const TransformComponent3D, const StaticMeshComponent3D>();

			mRenderables.clear();

			// Iterate 2D objects
			for (auto [entity, transform, mesh] : meshView2D.each())
			{
				const auto nodeID = enttSubsystem->GetID(entity);

				if (mesh.materialID == gInvalidID || mesh.meshID == gInvalidID)
				{
					continue;
				}

				const auto& matData = materialRegistry->GetMaterialData(mesh.materialID);

				mRenderables.emplace_back(nodeID, mesh.meshID, matData.baseMaterialID, mesh.subMeshIdx);

				if (!mCachedObjectData.Contains(nodeID))
				{
					mCachedObjectData.Emplace(nodeID, GPUObjectData());
				}
			}

			// Iterate 3D objects
			for (auto [entity, transform, mesh] : meshView3D.each())
			{
				const auto node_id = enttSubsystem->GetID(entity);

				if (mesh.materialID == gInvalidID || mesh.meshID == gInvalidID)
				{
					continue;
				}

				const auto& matData = materialRegistry->GetMaterialData(mesh.materialID);

				mRenderables.emplace_back(node_id, mesh.meshID, matData.baseMaterialID, mesh.subMeshIdx);

				if (!mCachedObjectData.Contains(node_id))
				{
					mCachedObjectData.Emplace(node_id, GPUObjectData());
				}
			}

			std::sort(mRenderables.begin(), mRenderables.end());

			for (auto& frameData : mFrameRenderData)
			{
				frameData.copyObjectDataToGPU = true;
			}
			
			mUpdateRenderables = false;
		}
	}

	void CoreRenderModuleVK::UpdateTextureDescriptors()
	{
		if (mInitialized && GetCurrentFrameData().textureDescriptorNeedsUpdated)
		{
			std::vector<vk::DescriptorImageInfo> textureImageInfos;
			BuildTextureDescriptorInfo(mTexData, textureImageInfos);

			util::DescriptorBuilder::Begin(mRenderSubsystem->GetDescriptorLayoutCache(), mRenderSubsystem->GetDescriptorAllocator())
				.UpdateImages(1, textureImageInfos.size(), textureImageInfos.data(),
							  vk::DescriptorType::eCombinedImageSampler)
				.Update(GetCurrentFrameData().materialDescriptor);

			GetCurrentFrameData().textureDescriptorNeedsUpdated = false;
		}
	}

	void CoreRenderModuleVK::BuildTextureDescriptorInfo(MappedVector<UUID, TextureDataVK>& textureData,
		std::vector<vk::DescriptorImageInfo>& textureImageInfos) const
	{
		textureImageInfos.clear();
		textureImageInfos.reserve(textureData.Size());

		const auto& textureManager = mRenderSubsystem->GetTextureManager();

		int idx = 0;
		for (auto& texData : textureData)
		{
			const auto& texture = textureManager->GetTexture(texData.id);
			
			vk::DescriptorImageInfo textureImageInfo = {
				texData.sampler, texture.imageView, vk::ImageLayout::eShaderReadOnlyOptimal
			};
			textureImageInfos.push_back(textureImageInfo);

			texData.idx = idx;
			idx++;
		}
	}

	void CoreRenderModuleVK::PrepareSceneData()
	{

	}

	void CoreRenderModuleVK::BuildIndirectCommands()
	{

	}

	CoreRenderModuleVK::FrameRenderData& CoreRenderModuleVK::GetFrameData(uint8_t frameIdx)
	{
		return mFrameRenderData[frameIdx];
	}

	CoreRenderModuleVK::FrameRenderData& CoreRenderModuleVK::GetCurrentFrameData()
	{
		return GetFrameData(mRenderSubsystem->GetCurrentFrameIdx());
	}
}
