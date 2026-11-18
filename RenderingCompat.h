#pragma once

#include "Core/EngineInfo.h"

#include <string>

namespace avd::compat
{
	struct Report
	{
		// A module from the Community Shaders family is loaded. Bottled Shaders is a fork of Community
		// Shaders (verified in Phase 1) and is believed to ship under the same DLL name, so the two
		// cannot be told apart by name alone. Both are handled by the same conservative policy.
		bool        shadersFamilyLoaded{ false };
		std::string shadersModule;

		// From Bottled Shaders PR #7: the BSAccumProcess::RegisterObject hook feeding Hi-Z occlusion
		// is only installed on AE before 1.7.99. On other runtimes Hi-Z culling is inert.
		bool hizCullingHookLikelyOwnedByShaders{ false };

		// Phase 2 installs no rendering hooks at all. This flag is the single gate future phases must
		// check before ever installing a culling/depth hook; it stays false when the shaders family is present.
		bool allowRenderingHooks{ false };

		std::string notes;
	};

	Report Detect(const core::EngineInfo& a_engine);

	void LogReport(const Report& a_report);
}
