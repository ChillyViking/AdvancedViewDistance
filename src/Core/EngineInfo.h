#pragma once

#include <string>

namespace avd::core
{
	struct EngineInfo
	{
		std::string versionText;  // e.g. "1.6.1170.0"
		bool isSE{ false };       // pre-AE 1.5.x
		bool isAE{ false };       // 1.6.x and later flat runtimes
		bool isVR{ false };
		bool supported{ false };  // whether this plugin will touch engine state on this runtime
		std::string reason;       // human-readable support decision
	};

	// Detects the running executable. Never throws.
	EngineInfo QueryEngine();
}
