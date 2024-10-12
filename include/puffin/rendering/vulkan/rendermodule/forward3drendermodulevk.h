#pragma once

#include "puffin/rendering/resourceid.h"
#include "puffin/rendering/vulkan/pipelinevk.h"
#include "vulkan/vulkan.hpp"

#include "puffin/rendering/vulkan/rendermodule/rendermodulevk.h"

namespace puffin::rendering
{
	/*
	 * Render module implementation for a 3d forward rendering pass
	 */
	class Forward3DRenderModuleVK : public RenderModuleVK
	{
	public:

		explicit Forward3DRenderModuleVK(std::shared_ptr<core::Engine> engine, RenderSubsystemVK* renderSubsystem);
		~Forward3DRenderModuleVK() override = default;

		void RegisterModules() override;

		void Initialize() override;
		void Deinitialize() override;
		void PostInitialize() override;

		void UpdateResources(ResourceManagerVK* resourceManager) override;

		void UpdateGraph(RenderGraphVK& renderGraph) override;

		void PreRender(double deltaTime) override;
		
		void RecordForward3DCommands(vk::CommandBuffer& cmd);

	private:

		void InitAttachments();
		void InitPipelineLayout();
		void InitDefaultForwardPipeline();

		vk::Format mColorFormat, mDepthFormat;

		ResourceID mColorResourceID = gInvalidID;
		ResourceID mDepthResourceID = gInvalidID;

		vk::UniquePipelineLayout mForwardPipelineLayout;

		util::ShaderModule mDefaultForwardVertMod, mDefaultForwardFragMod;
		vk::UniquePipeline mDefaultForwardPipeline;

	};
}