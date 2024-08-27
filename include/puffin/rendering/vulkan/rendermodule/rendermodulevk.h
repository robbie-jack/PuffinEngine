#pragma once

namespace puffin::rendering
{
	class RenderGraphVK;
	class RenderSubsystemVK;

	/*
	 * A render module is used to define & implement rendering techniques & effects
	 * i.e forward/deferred rendering, shadows, ambient occlusion, etc...
	 */
	class RenderModuleVK
	{
	public:

		explicit RenderModuleVK(RenderSubsystemVK* renderSubsystem);
		virtual ~RenderModuleVK();

		virtual void Initialize();
		virtual void Deinitialize();

		/*
		 * Called by render subsystem to build render graph, adding any render passes and binding callbacks for this module
		 */
		virtual void BuildGraph(RenderGraphVK& renderGraph);

		/*
		 * Called by render subsystem once a frame before rendering so module can update any required data/resources needed for rendering
		 */
		virtual void PreRender(double deltaTime);

	protected:

		RenderSubsystemVK* mRenderSubsystem = nullptr;

	};
}
