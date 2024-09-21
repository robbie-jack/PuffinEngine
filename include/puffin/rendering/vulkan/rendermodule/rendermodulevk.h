#pragma once

#include <memory>

namespace puffin
{
	namespace core
	{
		class Engine;
	}
}

namespace puffin::rendering
{
	class ResourceManagerVK;
	class RenderGraphVK;
	class RenderSubsystemVK;

	/*
	 * A render module is used to define & implement rendering techniques & effects
	 * i.e forward/deferred rendering, shadows, ambient occlusion, etc...
	 */
	class RenderModuleVK
	{
	public:

		explicit RenderModuleVK(std::shared_ptr<core::Engine> engine, RenderSubsystemVK* renderSubsystem);
		virtual ~RenderModuleVK();

		/*
		 * Register any modules this module requires
		 */
		virtual void RegisterModules();

		/*
		 * Initialize this module, create any needed resources (Images, buffers, descriptors, etc...)
		 */
		virtual void Initialize();

		/*
		 * Deinitialize this module, destroy any resources that are no longer needed
		 */
		virtual void Deinitialize();

		/*
		 * Called by render subsystem once a frame, create, update or destroy any resources as needed
		 */
		virtual void UpdateResources(ResourceManagerVK* resourceManager);

		/*
		 * Called by render subsystem to build render graph, adding any render passes and binding callbacks for this module
		 */
		virtual void UpdateGraph(RenderGraphVK& renderGraph);

		/*
		 * Called by render subsystem once a frame before rendering so module can update any data/resources required for rendering
		 */
		virtual void PreRender(double deltaTime);

	protected:

		std::shared_ptr<core::Engine> mEngine = nullptr;
		RenderSubsystemVK* mRenderSubsystem = nullptr;

	};
}
