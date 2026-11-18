#pragma once

namespace avd::logging
{
	// Creates "<Documents>/My Games/Skyrim.../SKSE/AdvancedViewDistance.log" via SKSE's log directory.
	// Safe to call once; on failure logging silently degrades (the plugin keeps running).
	void Init(bool a_debug);

	// Raises or lowers verbosity after the config has been read.
	void SetDebug(bool a_debug);
}
