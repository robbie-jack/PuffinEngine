#pragma once

struct BaseComponent
{
	bool bFlagCreated; // Indicates this component needs to be initialized
	bool bFlagDeleted; // Indicates this component needs to be cleaned up
};