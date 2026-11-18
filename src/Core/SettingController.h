#pragma once

#include "Configuration/ConfigModel.h"

#include <string>
#include <unordered_map>

namespace avd::core
{
	// Drives engine INI Setting objects (Tier A in the Phase 1 report). No hooks, no memory patches:
	// it only writes the float value of Settings the engine itself exposes via RE::GetINISetting.
	//
	// Every written value is baseline * multiplier, where baseline is captured ONCE before the first write,
	// so repeated Apply calls never compound.
	class SettingController
	{
	public:
		static SettingController& Get();

		// Reads and stores the engine's current values. Idempotent.
		void CaptureBaseline();

		// Applies multipliers. With a_dryRun the intended values are logged and nothing is written.
		void Apply(const Multipliers& a_multipliers, bool a_dryRun);

		// Logs current vs baseline for every managed setting (diagnostics).
		void LogState() const;

	private:
		SettingController() = default;

		std::unordered_map<std::string, float> baseline_;
		std::unordered_map<std::string, float> lastApplied_;
		bool captured_{ false };
	};
}
