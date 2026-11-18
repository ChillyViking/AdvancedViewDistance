#include "PCH.h"

#include "Configuration/Config.h"

namespace avd::config
{
	Config LoadFromDisk()
	{
		std::vector<std::string> warnings;
		Config cfg;

		std::ifstream file(kIniPath);
		if (!file.is_open()) {
			logger::warn("Config: '{}' not found or unreadable; using built-in defaults", kIniPath);
		} else {
			cfg = LoadConfig(file, warnings);
			logger::info("Config: loaded '{}'", kIniPath);
		}

		for (const auto& w : warnings) {
			logger::warn("Config: {}", w);
		}
		return cfg;
	}

	void LogConfig(const Config& a_cfg, const Multipliers& a_eff)
	{
		logger::info("Config: enabled={} dryRun={} debug={} preset={} global={:.2f} maxMult={:.2f}",
			a_cfg.enabled, a_cfg.dryRun, a_cfg.debugLogging, PresetName(a_cfg.preset),
			a_cfg.globalMultiplier, a_cfg.maxMultiplier);
		logger::info("Config: effective multipliers objects={:.2f} items={:.2f} actors={:.2f} treeLoad={:.2f} terrainL0={:.2f} terrainL1={:.2f}",
			a_eff.objects, a_eff.items, a_eff.actors, a_eff.treeLoad, a_eff.terrainLevel0, a_eff.terrainLevel1);
	}
}
