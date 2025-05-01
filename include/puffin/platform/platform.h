#pragma once

#include <memory>
#include <string>

namespace puffin::core
{
	class Engine;

	/*
	 * Return time since engine launch in seconds
	 */
	double GetTime();

	class Platform
	{
	public:

		Platform(std::shared_ptr<Engine> engine);
		virtual ~Platform() = default;

		/*
		 * Register any application specific types (subsystems, components, nodes, etc...) here, gets called before initialization
		 */
		virtual void RegisterTypes() = 0;

	protected:

		std::string mName;
		std::shared_ptr<Engine> mEngine = nullptr;

	private:



	};
}
