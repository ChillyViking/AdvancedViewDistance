#include "PCH.h"

#include "Core/EngineInfo.h"

namespace avd::core
{
	EngineInfo QueryEngine()
	{
		EngineInfo info;
		try {
			// CommonLibSSE-NG runtime detection (verify these static helpers against the pinned release).
			info.versionText = REL::Module::get().version().string();
			info.isVR = REL::Module::IsVR();
			info.isAE = REL::Module::IsAE();
			info.isSE = REL::Module::IsSE();
		} catch (...) {
			info.supported = false;
			info.reason = "runtime detection threw; refusing to touch engine state";
			return info;
		}

		if (info.isVR) {
			// VR uses different Address Library data and has not been analysed in Phase 1.
			info.supported = false;
			info.reason = "Skyrim VR is not supported yet (untested)";
		} else if (info.isSE || info.isAE) {
			info.supported = true;
			info.reason = info.isAE ? "Skyrim AE runtime" : "Skyrim SE runtime";
		} else {
			info.supported = false;
			info.reason = "unrecognised runtime";
		}
		return info;
	}
}
