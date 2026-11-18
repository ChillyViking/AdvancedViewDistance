#include "PCH.h"

#include "Plugin/Plugin.h"

#include "Compatibility/RenderingCompat.h"
#include "Configuration/Config.h"
#include "Core/EngineInfo.h"
#include "Core/SettingController.h"
#include "Logging/Log.h"

namespace avd::plugin
{
	namespace
	{
		struct State
		{
			Config           config;
			Multipliers      effective;
			core::EngineInfo engine;
			compat::Report   compat;
			bool             active{ false };  // true once settings control is live
		};

		State g_state;
	}

	void OnDataLoaded()
	{
		try {
			g_state.config = config::LoadFromDisk();
			logging::SetDebug(g_state.config.debugLogging);
			g_state.effective = Effective(g_state.config);
			config::LogConfig(g_state.config, g_state.effective);

			g_state.engine = core::QueryEngine();
			logger::info("Engine: version {} ({})", g_state.engine.versionText, g_state.engine.reason);

			g_state.compat = compat::Detect(g_state.engine);
			compat::LogReport(g_state.compat);

			if (!g_state.config.enabled) {
				logger::info("Plugin disabled by config (bEnabled=false); no engine state touched");
				return;
			}
			if (!g_state.engine.supported) {
				logger::warn("Unsupported runtime ({}); no engine state touched", g_state.engine.reason);
				return;
			}

			auto& settings = core::SettingController::Get();
			settings.CaptureBaseline();
			settings.Apply(g_state.effective, g_state.config.dryRun);
			g_state.active = !g_state.config.dryRun;
			logger::info("Initialisation complete (active={}, dryRun={})", g_state.active, g_state.config.dryRun);
		} catch (const std::exception& e) {
			logger::error("OnDataLoaded failed: {}", e.what());
		} catch (...) {
			logger::error("OnDataLoaded failed with an unknown exception");
		}
	}

	void OnGameStart()
	{
		try {
			if (!g_state.active) {
				return;
			}
			auto& settings = core::SettingController::Get();
			settings.Apply(g_state.effective, false);
			settings.LogState();
		} catch (const std::exception& e) {
			logger::error("OnGameStart failed: {}", e.what());
		} catch (...) {
			logger::error("OnGameStart failed with an unknown exception");
		}
	}
}
