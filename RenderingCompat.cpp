#include "PCH.h"

#include "Compatibility/RenderingCompat.h"

namespace avd::compat
{
	namespace
	{
		// ASSUMPTION (not verified): Community Shaders and its forks ship as this DLL. If detection
		// fails, the plugin still behaves safely because it installs no hooks in Phase 2.
		constexpr const char* kShadersModule = "CommunityShaders.dll";

		bool IsAtLeastAE1799()
		{
			// Bottled Shaders PR #7 uses "IsAE() && !IsAtLeastAE1799()" in Hooks.cpp; we mirror the threshold.
			return REL::Module::get().version() >= REL::Version(1, 7, 99, 0);
		}
	}

	Report Detect(const core::EngineInfo& a_engine)
	{
		Report r;

		try {
			if (::GetModuleHandleA(kShadersModule) != nullptr) {
				r.shadersFamilyLoaded = true;
				r.shadersModule = kShadersModule;
			}
		} catch (...) {
			r.notes += "module probe threw; assuming nothing about other renderers. ";
		}

		if (r.shadersFamilyLoaded && a_engine.isAE && !IsAtLeastAE1799()) {
			r.hizCullingHookLikelyOwnedByShaders = true;
		}

		// Phase 2 policy: never install rendering hooks. Later phases may only flip this to true
		// when no shaders-family module is loaded and the specific hook is not already owned.
		r.allowRenderingHooks = false;

		if (r.shadersFamilyLoaded) {
			if (r.hizCullingHookLikelyOwnedByShaders) {
				r.notes += "Community Shaders family detected; its Hi-Z occlusion hook is expected active on this "
						   "runtime. This plugin installs no culling hooks and only adjusts engine INI settings.";
			} else {
				r.notes += "Community Shaders family detected; Hi-Z culling is expected inert on this runtime "
						   "(per Bottled Shaders PR #7). This plugin installs no hooks and only adjusts engine INI settings.";
			}
		} else {
			r.notes += "No Community Shaders family module detected.";
		}
		return r;
	}

	void LogReport(const Report& a_r)
	{
		logger::info("Compat: shadersFamilyLoaded={} module='{}' hizHookLikelyOwnedByShaders={} allowRenderingHooks={}",
			a_r.shadersFamilyLoaded, a_r.shadersModule, a_r.hizCullingHookLikelyOwnedByShaders, a_r.allowRenderingHooks);
		logger::info("Compat: {}", a_r.notes);
	}
}
