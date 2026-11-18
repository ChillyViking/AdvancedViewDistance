#pragma once

// Pure configuration model: presets, clamping, effective-multiplier math.
// No engine dependencies, so it is unit-tested off-platform (tests/).

#include "Configuration/IniParser.h"

#include <istream>
#include <optional>
#include <string>
#include <vector>

namespace avd
{
	enum class Preset
	{
		Low,
		Balanced,
		High,
		Ultra
	};

	// Multipliers relative to the engine value captured at startup.
	// 1.0 == leave the engine value unchanged.
	struct Multipliers
	{
		float objects{ 1.0f };        // fLODFadeOutMultObjects:LOD
		float items{ 1.0f };          // fLODFadeOutMultItems:LOD
		float actors{ 1.0f };         // fLODFadeOutMultActors:LOD
		float treeLoad{ 1.0f };       // fTreeLoadDistance:TerrainManager
		float terrainLevel0{ 1.0f };  // fBlockLevel0Distance:TerrainManager
		float terrainLevel1{ 1.0f };  // fBlockLevel1Distance:TerrainManager
	};

	// NOTE: preset values are INITIAL GUESSES that must be tuned by in-game testing.
	inline Multipliers PresetMultipliers(Preset a_preset)
	{
		switch (a_preset) {
		case Preset::Low:
			return { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
		case Preset::Balanced:
			return { 1.5f, 1.25f, 1.25f, 1.5f, 1.5f, 1.5f };
		case Preset::High:
			return { 2.0f, 1.5f, 1.5f, 2.0f, 2.0f, 2.0f };
		case Preset::Ultra:
			return { 3.0f, 2.0f, 2.0f, 3.0f, 3.0f, 3.0f };
		}
		return {};
	}

	inline const char* PresetName(Preset a_preset)
	{
		switch (a_preset) {
		case Preset::Low:
			return "Low";
		case Preset::Balanced:
			return "Balanced";
		case Preset::High:
			return "High";
		case Preset::Ultra:
			return "Ultra";
		}
		return "Unknown";
	}

	inline std::optional<Preset> ParsePreset(const std::string& a_name)
	{
		const auto n = ini::Lower(ini::Trim(a_name));
		if (n == "low") {
			return Preset::Low;
		}
		if (n == "balanced") {
			return Preset::Balanced;
		}
		if (n == "high") {
			return Preset::High;
		}
		if (n == "ultra") {
			return Preset::Ultra;
		}
		return std::nullopt;
	}

	inline constexpr float kMinMultiplier = 0.25f;
	inline constexpr float kHardMaxMultiplier = 8.0f;

	// Clamps a multiplier into [kMinMultiplier, maxMult]; maxMult itself is bounded by
	// [1, kHardMaxMultiplier]. Non-finite input yields 1.0 (no change).
	inline float ClampMultiplier(float a_value, float a_maxMult)
	{
		if (!std::isfinite(a_value)) {
			return 1.0f;
		}
		const float maxMult = std::isfinite(a_maxMult) ? std::min(std::max(a_maxMult, 1.0f), kHardMaxMultiplier) : 1.0f;
		return std::min(std::max(a_value, kMinMultiplier), maxMult);
	}

	struct Config
	{
		bool enabled{ true };
		bool dryRun{ false };  // log what WOULD be applied, change nothing
		bool debugLogging{ false };
		Preset preset{ Preset::Balanced };
		float globalMultiplier{ 1.0f };
		float maxMultiplier{ 4.0f };

		// Per-setting overrides. When set they REPLACE the preset value; the global
		// multiplier is applied afterwards in all cases.
		std::optional<float> objects;
		std::optional<float> items;
		std::optional<float> actors;
		std::optional<float> treeLoad;
		std::optional<float> terrainLevel0;
		std::optional<float> terrainLevel1;
	};

	// Parses configuration. Never throws; problems are reported via a_warnings and
	// the affected key falls back to its default.
	inline Config LoadConfig(std::istream& a_in, std::vector<std::string>& a_warnings)
	{
		const auto doc = ini::Parse(a_in);
		Config cfg;

		const auto readBool = [&](const char* sec, const char* key, bool& out) {
			if (ini::GetString(doc, sec, key)) {
				if (const auto v = ini::GetBool(doc, sec, key)) {
					out = *v;
				} else {
					a_warnings.push_back(std::string("[") + sec + "] " + key + ": not a boolean, using default");
				}
			}
		};
		const auto readFloat = [&](const char* sec, const char* key, float& out) {
			if (ini::GetString(doc, sec, key)) {
				if (const auto v = ini::GetFloat(doc, sec, key)) {
					out = *v;
				} else {
					a_warnings.push_back(std::string("[") + sec + "] " + key + ": not a number, using default");
				}
			}
		};
		const auto readOpt = [&](const char* sec, const char* key, std::optional<float>& out) {
			if (ini::GetString(doc, sec, key)) {
				if (const auto v = ini::GetFloat(doc, sec, key)) {
					out = *v;
				} else {
					a_warnings.push_back(std::string("[") + sec + "] " + key + ": not a number, ignoring override");
				}
			}
		};

		readBool("General", "bEnabled", cfg.enabled);
		readBool("General", "bDryRun", cfg.dryRun);
		readBool("General", "bDebugLogging", cfg.debugLogging);
		readFloat("General", "fGlobalMultiplier", cfg.globalMultiplier);
		readFloat("Safety", "fMaxMultiplier", cfg.maxMultiplier);

		if (const auto name = ini::GetString(doc, "General", "sPreset")) {
			if (const auto p = ParsePreset(*name)) {
				cfg.preset = *p;
			} else {
				a_warnings.push_back("[General] sPreset: unknown preset '" + *name + "', using Balanced");
			}
		}

		readOpt("Objects", "fObjectFadeMult", cfg.objects);
		readOpt("Objects", "fItemFadeMult", cfg.items);
		readOpt("Actors", "fActorFadeMult", cfg.actors);
		readOpt("Environment", "fTreeLoadDistanceMult", cfg.treeLoad);
		readOpt("LOD", "fTerrainBlockLevel0Mult", cfg.terrainLevel0);
		readOpt("LOD", "fTerrainBlockLevel1Mult", cfg.terrainLevel1);

		return cfg;
	}

	// Final multipliers: (override or preset) * global, clamped.
	inline Multipliers Effective(const Config& a_cfg)
	{
		const auto base = PresetMultipliers(a_cfg.preset);
		const auto pick = [&](const std::optional<float>& over, float presetValue) {
			const float raw = over ? *over : presetValue;
			return ClampMultiplier(raw * a_cfg.globalMultiplier, a_cfg.maxMultiplier);
		};
		Multipliers m;
		m.objects = pick(a_cfg.objects, base.objects);
		m.items = pick(a_cfg.items, base.items);
		m.actors = pick(a_cfg.actors, base.actors);
		m.treeLoad = pick(a_cfg.treeLoad, base.treeLoad);
		m.terrainLevel0 = pick(a_cfg.terrainLevel0, base.terrainLevel0);
		m.terrainLevel1 = pick(a_cfg.terrainLevel1, base.terrainLevel1);
		return m;
	}
}
