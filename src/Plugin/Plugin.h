#pragma once

namespace avd::plugin
{
	// kDataLoaded: read config, detect runtime and compatibility, capture baseline, apply.
	void OnDataLoaded();

	// kPostLoadGame / kNewGame: re-apply (cheap, idempotent) and log state for diagnostics.
	void OnGameStart();
}
