#include "PCH.h"

#include "Core/SettingController.h"

#include <array>
#include <cmath>

namespace avd::core
{
	namespace
	{
		struct Target
		{
			const char*      name;
			float Multipliers::* member;
		};

		// Setting names verified in Phase 1 from SSE FPS Stabilizer usage (see PHASE1_TECHNICAL_REPORT.md).
		// A name that does not resolve at runtime is skipped with a warning, never fatal.
		constexpr std::array kTargets{
			Target{ "fLODFadeOutMultObjects:LOD", &Multipliers::objects },
			Target{ "fLODFadeOutMultItems:LOD", &Multipliers::items },
			Target{ "fLODFadeOutMultActors:LOD", &Multipliers::actors },
			Target{ "fTreeLoadDistance:TerrainManager", &Multipliers::treeLoad },
			Target{ "fBlockLevel0Distance:TerrainManager", &Multipliers::terrainLevel0 },
			Target{ "fBlockLevel1Distance:TerrainManager", &Multipliers::terrainLevel1 },
		};

		// Returns the float setting or nullptr if missing / not a float.
		// RE::GetINISetting and Setting::data.f / GetType() are CommonLibSSE API; verify at first compile.
		RE::Setting* FindFloatSetting(const char* a_name)
		{
			auto* setting = RE::GetINISetting(a_name);
			if (!setting) {
				logger::warn("Settings: '{}' not found in this runtime; skipping", a_name);
				return nullptr;
			}
			if (setting->GetType() != RE::Setting::Type::kFloat) {
				logger::warn("Settings: '{}' is not a float setting; skipping", a_name);
				return nullptr;
			}
			return setting;
		}
	}

	SettingController& SettingController::Get()
	{
		static SettingController instance;
		return instance;
	}

	void SettingController::CaptureBaseline()
	{
		if (captured_) {
			return;
		}
		for (const auto& t : kTargets) {
			if (auto* s = FindFloatSetting(t.name)) {
				baseline_[t.name] = s->data.f;
				logger::info("Settings: baseline {} = {:.3f}", t.name, s->data.f);
			}
		}
		captured_ = true;
	}

	void SettingController::Apply(const Multipliers& a_m, bool a_dryRun)
	{
		if (!captured_) {
			CaptureBaseline();
		}

		for (const auto& t : kTargets) {
			const auto base = baseline_.find(t.name);
			if (base == baseline_.end()) {
				continue;
			}
			auto* s = FindFloatSetting(t.name);
			if (!s) {
				continue;
			}

			const float target = base->second * (a_m.*(t.member));
			if (!std::isfinite(target) || target <= 0.0f) {
				logger::warn("Settings: computed value for {} is invalid ({}); leaving unchanged", t.name, target);
				continue;
			}

			const float current = s->data.f;
			if (const auto last = lastApplied_.find(t.name);
				last != lastApplied_.end() && std::fabs(current - last->second) > 1e-3f) {
				logger::warn("Settings: {} was changed externally ({:.3f} -> {:.3f}) since we wrote it; re-applying",
					t.name, last->second, current);
			}

			if (a_dryRun) {
				logger::info("Settings: [dry-run] {} would change {:.3f} -> {:.3f}", t.name, current, target);
				continue;
			}

			s->data.f = target;
			lastApplied_[t.name] = target;
			logger::info("Settings: {} {:.3f} -> {:.3f}", t.name, current, target);
		}
	}

	void SettingController::LogState() const
	{
		for (const auto& t : kTargets) {
			const auto base = baseline_.find(t.name);
			auto*      s = RE::GetINISetting(t.name);
			if (base != baseline_.end() && s && s->GetType() == RE::Setting::Type::kFloat) {
				logger::info("Settings: state {} baseline={:.3f} current={:.3f}", t.name, base->second, s->data.f);
			}
		}
	}
}
