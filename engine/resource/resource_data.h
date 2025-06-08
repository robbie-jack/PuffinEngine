#pragma once

namespace puffin
{
	/*
	 * Stores loaded resource data in a common format to be used by engine code
	 *
	 */
	class ResourceData
	{
	public:

		virtual ~ResourceData() = 0;

		virtual bool IsLoaded() const = 0;

	private:



	};
}