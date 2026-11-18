#include "PCH.h"

#include "Logging/Log.h"

namespace avd::logging
{
	void Init(bool a_debug)
	{
		try {
			auto path = logger::log_directory();
			if (!path) {
				return;
			}
			*path /= "AdvancedViewDistance.log"sv;
			auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
			auto log = std::make_shared<spdlog::logger>("avd"s, std::move(sink));
			spdlog::set_default_logger(std::move(log));
			spdlog::set_pattern("[%Y-%m-%d %T.%e] [%l] %v"s);
			SetDebug(a_debug);
			spdlog::flush_on(spdlog::level::info);
		} catch (...) {
			// Logging must never take the game down.
		}
	}

	void SetDebug(bool a_debug)
	{
		spdlog::set_level(a_debug ? spdlog::level::debug : spdlog::level::info);
	}
}
