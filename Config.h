#pragma once

#include "Configuration/ConfigModel.h"

namespace avd::config
{
	inline constexpr auto kIniPath = "Data/SKSE/Plugins/AdvancedViewDistance.ini";

	// Loads the INI from disk. A missing or unreadable file yields defaults (logged); never throws.
	Config LoadFromDisk();

	// Writes the effective configuration to the log.
	void LogConfig(const Config& a_cfg, const Multipliers& a_effective);
}
