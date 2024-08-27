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
		 * Called by render subsystem once a frame so module can update its internal data, ready for next frame
		 */
		virtual void Update(double deltaTime);

		/*
		 * Called by render subsystem to prepare render graph, adding any render passes and binding callbacks for this module
		 */
		virtual void PrepareGraph(RenderGraphVK& renderGraph);

	protected:

		RenderSubsystemVK* mRenderSubsystem = nullptr;

	};
}
