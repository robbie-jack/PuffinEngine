#pragma once

#ifndef BASE_COMPONENT_H
#define BASE_COMPONENT_H

namespace Puffin
{
	struct BaseComponent
	{
		bool flag_created = false; // Component was just created
		//bool flag_recreate = false; // Component has had it's data chnaged and needs to be recreated
		bool flag_deleted = false; // Component has been marked for deletion
	};
}

#endif // BASE_COMPONENT_H