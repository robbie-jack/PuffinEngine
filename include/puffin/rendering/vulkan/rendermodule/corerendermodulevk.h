#pragma once

#include <unordered_set>

#include "vulkan/vulkan.hpp"

#include "puffin/rendering/resourceid.h"
#include "puffin/rendering/vulkan/rendermodule/rendermodulevk.h"
#include "entt/entity/registry.hpp"
#include "puffin/rendering/renderglobals.h"
#include "puffin/rendering/vulkan/typesvk.h"
#include "puffin/types/vector3.h"
#include "puffin/types/storage/mappedvector.h"

namespace puffin::rendering
{
	/*
	 *	Render module implementation for core rendering functionality,
	 *	shared between different rendering modules
	 */
	class CoreRenderModuleVK : public RenderModuleVK
	{
	public:
		
		struct FrameRenderData
		{
			vk::DescriptorSet objectDescriptor;
			vk::DescriptorSet lightDescriptor;
			vk::DescriptorSet materialDescriptor;
			vk::DescriptorSet shadowDescriptor;

			GPUFragShaderPushConstant pushConstantFrag;

			bool updateGPUObjectData = false;
			bool updateGPUMaterialData = false;
			bool updateGPULightData = false;
			bool textureDescriptorNeedsUpdated = false;
		};

		explicit CoreRenderModuleVK(std::shared_ptr<core::Engine> engine, RenderSubsystemVK* renderSubsystem);
		~CoreRenderModuleVK() override = default;

		void RegisterModules() override;

		void Initialize() override;
		void Deinitialize() override;

		void PostInitialize() override;

		void UpdateResources(ResourceManagerVK* resourceManager) override;

		void UpdateGraph(RenderGraphVK& renderGraph) override;

		void PreRender(double deltaTime) override;

		static void RecordCopyCommand(vk::CommandBuffer& cmd, const AllocatedImage& imageToCopy,
			const vk::Image& swapchainImage, const vk::Extent2D& extent);

		FrameRenderData& GetFrameData(uint8_t frameIdx);
		FrameRenderData& GetCurrentFrameData();
		std::vector<MeshDrawBatch>& GetMeshDrawBatches();

	private:

		void OnUpdateMesh(entt::registry& registry, entt::entity entity);
		void OnUpdateTransform(entt::registry& registry, entt::entity entity);
		void OnDestroyMeshOrTransform(entt::registry& registry, entt::entity entity);
		void OnUpdateLight(entt::registry& registry, entt::entity entity);

		void AddRenderable(entt::registry& registry, entt::entity entity);

		void BindCallbacks();
		void InitBuffers();
		void InitSamplers();
		void InitDescriptorLayouts();
		void InitDescriptorSets();
		
		void ProcessComponents();
		
		void UpdateTextureDescriptors();
		void BuildTextureDescriptorInfo(MappedVector<UUID, TextureDataVK>& textureData,
												   std::vector<vk::DescriptorImageInfo>& textureImageInfos) const;
		
		void PrepareSceneData();
		void PrepareMaterialData();
		void PrepareObjectData();
		void PrepareLightData();
		
		void BuildIndirectCommands();

		bool mInitialized = false; // Indicates initialization completed without any failures
		bool mUpdateRenderables = false; // Indicate renderables need to be updated

		vk::Sampler mTextureSampler;

		ResourceID mIndirectDrawBufferID = gInvalidID;
		ResourceID mCameraBufferID = gInvalidID;
		ResourceID mPointLightBufferID = gInvalidID;
		ResourceID mSpotLightBufferID = gInvalidID;
		ResourceID mDirLightBufferID = gInvalidID;
		ResourceID mObjectBufferID = gInvalidID;
		ResourceID mMaterialInstanceBufferID = gInvalidID;

		ResourceID mObjectDescriptorLayoutID = gInvalidID;
		ResourceID mLightDescriptorLayoutID = gInvalidID;
		ResourceID mMatDescriptorLayoutID = gInvalidID;
		ResourceID mShadowDescriptorLayoutID = gInvalidID; // PUFFIN_TODO - Dummy layout, will be removed once shadows are implemented in own render module

		std::array<FrameRenderData, gBufferedFrameCount> mFrameRenderData;
		std::unordered_set<UUID> mMeshesToLoad; // Meshes that need to be loaded
		
		MappedVector<UUID, TextureDataVK> mTexData;

		std::vector<MeshRenderable> mRenderables;

		std::vector<MeshDrawBatch> mDrawBatches;

		MappedVector<UUID, GPUObjectData> mCachedObjectData; // Cached data for rendering each object in scene
		std::unordered_set<UUID> mObjectsToUpdate; // Objects which need their mesh data refreshed

		MappedVector<UUID, Vector3f> mCachedLightDirection;
	};
}
