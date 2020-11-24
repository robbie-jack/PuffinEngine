#pragma once

namespace Puffin
{
	struct BaseComponent
	{
		bool flag_created; // Component was just created
		bool flag_recreate; // Component has had it's data chnaged and needs to be recreated
		bool flag_deleted; // Component has been marked for deletion
	};
}