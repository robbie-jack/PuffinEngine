#pragma once

#include "vulkan/vulkan.hpp"

#include "puffin/rendering/resourceid.h"
#include "puffin/rendering/vulkan/rendermodule/rendermodulevk.h"

namespace puffin::rendering
{
	/*
	 *	Render module implementation for core rendering functionality,
	 *	shared between different rendering modules
	 */
	class CoreRenderModuleVK : public RenderModuleVK
	{
	public:

		explicit CoreRenderModuleVK(std::shared_ptr<core::Engine> engine, RenderSubsystemVK* renderSubsystem);
		~CoreRenderModuleVK() override = default;

		void RegisterModules() override;

		void Initialize() override;
		void Deinitialize() override;

		void UpdateResources(ResourceManagerVK* resourceManager) override;

		void UpdateGraph(RenderGraphVK& renderGraph) override;

		void PreRender(double deltaTime) override;

	private:

		void InitBuffers();
		void InitSamplers();
		void InitDescriptors();

		vk::Sampler mTextureSampler;

		ResourceID mIndirectDrawBufferID = gInvalidID;
		ResourceID mCameraBufferID = gInvalidID;
		ResourceID mPointLightBufferID = gInvalidID;
		ResourceID mSpotLightBufferID = gInvalidID;
		ResourceID mDirLightBufferID = gInvalidID;
		ResourceID mObjectBufferID = gInvalidID;
		ResourceID mMaterialInstanceBufferID = gInvalidID;

		ResourceID mObjectDescriptorLayoutID = gInvalidID;
		ResourceID mGlobalDescriptorLayoutID = gInvalidID;
		ResourceID mTextureDescriptorLayoutID = gInvalidID;

	};
}