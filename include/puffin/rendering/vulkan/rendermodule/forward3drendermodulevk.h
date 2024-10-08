#pragma once

#include "puffin/rendering/resourceid.h"
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

		void UpdateResources(ResourceManagerVK* resourceManager) override;

		void UpdateGraph(RenderGraphVK& renderGraph) override;

		void PreRender(double deltaTime) override;
		
		void RecordForward3DCommands(vk::CommandBuffer& cmd);

	private:

		ResourceID mColorResourceID = gInvalidID;
		ResourceID mDepthResourceID = gInvalidID;

	};
}