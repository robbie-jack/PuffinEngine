#pragma once

#include "vulkan/vulkan.hpp"

#include "puffin/rendering/resourceid.h"
#include "puffin/rendering/vulkan/rendermodule/rendermodulevk.h"
#include "entt/entity/registry.hpp"
#include "puffin/rendering/renderglobals.h"

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

	private:

		void OnUpdateMesh(entt::registry& registry, entt::entity entity);
		void OnUpdateTransform(entt::registry& registry, entt::entity entity);
		void OnDestroyMeshOrTransform(entt::registry& registry, entt::entity entity);

		void AddRenderable(entt::registry& registry, entt::entity entity);

		void InitBuffers();
		void InitSamplers();
		void InitDescriptorLayouts();
		void InitDescriptorSets();

		void UpdateRenderData();
		void ProcessComponents();
		void UpdateTextureDescriptors();
		void PrepareSceneData();
		void BuildIndirectCommands();

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
	};
}
