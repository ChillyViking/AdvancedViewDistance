// Off-platform unit tests for the pure configuration logic.
// Build: g++ -std=c++20 -Wall -Wextra -I src tests/test_config.cpp -o test_config && ./test_config

#include "Configuration/ConfigModel.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <sstream>

static int g_failures = 0;
#define CHECK(cond)                                                                 \
	do {                                                                            \
		if (!(cond)) {                                                              \
			std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);             \
			++g_failures;                                                           \
		}                                                                           \
	} while (0)

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

static avd::Config Load(const char* text, std::vector<std::string>& warnings)
{
	std::istringstream in(text);
	return avd::LoadConfig(in, warnings);
}

int main()
{
	// defaults on empty input
	{
		std::vector<std::string> w;
		const auto c = Load("", w);
		CHECK(c.enabled);
		CHECK(!c.dryRun);
		CHECK(c.preset == avd::Preset::Balanced);
		CHECK(Near(c.globalMultiplier, 1.0f));
		CHECK(w.empty());
	}
	// parsing, case-insensitivity, comments, BOM, inline comments
	{
		std::vector<std::string> w;
		const auto c = Load("\xEF\xBB\xBF; header\n[general]\nBENABLED = false ; off\nsPreset=ULTRA\n"
							"fGlobalMultiplier = 1.5\n# c\n[Objects]\nfObjectFadeMult=2.5\n",
			w);
		CHECK(!c.enabled);
		CHECK(c.preset == avd::Preset::Ultra);
		CHECK(Near(c.globalMultiplier, 1.5f));
		CHECK(c.objects.has_value() && Near(*c.objects, 2.5f));
		CHECK(w.empty());
	}
	// invalid values warn and fall back
	{
		std::vector<std::string> w;
		const auto c = Load("[General]\nbEnabled=maybe\nsPreset=Insane\nfGlobalMultiplier=abc\n"
							"[Objects]\nfObjectFadeMult=nan\n",
			w);
		CHECK(c.enabled);
		CHECK(c.preset == avd::Preset::Balanced);
		CHECK(Near(c.globalMultiplier, 1.0f));
		CHECK(!c.objects.has_value());
		CHECK(w.size() == 4);
	}
	// trailing garbage / inf rejected
	{
		std::vector<std::string> w;
		const auto c = Load("[General]\nfGlobalMultiplier=2.0x\n[Actors]\nfActorFadeMult=inf\n", w);
		CHECK(Near(c.globalMultiplier, 1.0f));
		CHECK(!c.actors.has_value());
		CHECK(w.size() == 2);
	}
	// clamping
	{
		CHECK(Near(avd::ClampMultiplier(100.0f, 4.0f), 4.0f));
		CHECK(Near(avd::ClampMultiplier(0.0f, 4.0f), avd::kMinMultiplier));
		CHECK(Near(avd::ClampMultiplier(-5.0f, 4.0f), avd::kMinMultiplier));
		CHECK(Near(avd::ClampMultiplier(std::numeric_limits<float>::quiet_NaN(), 4.0f), 1.0f));
		CHECK(Near(avd::ClampMultiplier(std::numeric_limits<float>::infinity(), 4.0f), 1.0f));
		// max is itself bounded to the hard cap and floored at 1
		CHECK(Near(avd::ClampMultiplier(50.0f, 1000.0f), avd::kHardMaxMultiplier));
		CHECK(Near(avd::ClampMultiplier(2.0f, 0.1f), 1.0f));
	}
	// effective multipliers: preset, override replaces preset, global scales, clamp applies
	{
		std::vector<std::string> w;
		auto c = Load("[General]\nsPreset=High\n", w);
		auto m = avd::Effective(c);
		CHECK(Near(m.objects, 2.0f));
		CHECK(Near(m.items, 1.5f));

		c = Load("[General]\nsPreset=High\nfGlobalMultiplier=2\n[Objects]\nfObjectFadeMult=1.0\n", w);
		m = avd::Effective(c);
		CHECK(Near(m.objects, 2.0f));   // override 1.0 * global 2
		CHECK(Near(m.items, 3.0f));     // preset 1.5 * global 2
		CHECK(Near(m.treeLoad, 4.0f));  // preset 2.0 * global 2

		c = Load("[General]\nsPreset=Ultra\nfGlobalMultiplier=3\n[Safety]\nfMaxMultiplier=4\n", w);
		m = avd::Effective(c);
		CHECK(Near(m.objects, 4.0f));  // 9 clamped to 4
	}
	// Low preset is a no-op
	{
		std::vector<std::string> w;
		const auto m = avd::Effective(Load("[General]\nsPreset=Low\n", w));
		CHECK(Near(m.objects, 1.0f) && Near(m.items, 1.0f) && Near(m.actors, 1.0f));
		CHECK(Near(m.treeLoad, 1.0f) && Near(m.terrainLevel0, 1.0f) && Near(m.terrainLevel1, 1.0f));
	}
	// all presets stay within the default clamp
	{
		for (auto p : { avd::Preset::Low, avd::Preset::Balanced, avd::Preset::High, avd::Preset::Ultra }) {
			const auto m = avd::PresetMultipliers(p);
			CHECK(m.objects >= 1.0f && m.objects <= 4.0f);
			CHECK(m.terrainLevel1 >= 1.0f && m.terrainLevel1 <= 4.0f);
		}
	}

	if (g_failures == 0) {
		std::printf("ALL TESTS PASSED\n");
		return 0;
	}
	std::printf("%d FAILURE(S)\n", g_failures);
	return 1;
}
