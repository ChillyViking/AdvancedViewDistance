#pragma once

// Minimal, dependency-free INI parser. Deliberately has NO engine/CommonLib
// includes so it can be unit-tested off-platform.

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <istream>
#include <map>
#include <optional>
#include <string>

namespace avd::ini
{
	using Section = std::map<std::string, std::string>;
	using Document = std::map<std::string, Section>;

	inline std::string Trim(const std::string& a_s)
	{
		const auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
		auto begin = std::find_if_not(a_s.begin(), a_s.end(), isSpace);
		auto end = std::find_if_not(a_s.rbegin(), std::string::const_reverse_iterator(begin), isSpace).base();
		return begin < end ? std::string(begin, end) : std::string{};
	}

	inline std::string Lower(std::string a_s)
	{
		std::transform(a_s.begin(), a_s.end(), a_s.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return a_s;
	}

	// Section and key names are case-insensitive (stored lowercased).
	// Lines starting with ';' or '#' are comments; ';' after a value starts an inline comment.
	inline Document Parse(std::istream& a_in)
	{
		Document doc;
		std::string section;
		std::string line;
		bool first = true;
		while (std::getline(a_in, line)) {
			if (first) {
				first = false;
				if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
					static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF) {
					line.erase(0, 3);  // UTF-8 BOM
				}
			}
			line = Trim(line);
			if (line.empty() || line[0] == ';' || line[0] == '#') {
				continue;
			}
			if (line.front() == '[') {
				const auto close = line.find(']');
				if (close != std::string::npos) {
					section = Lower(Trim(line.substr(1, close - 1)));
				}
				continue;
			}
			const auto eq = line.find('=');
			if (eq == std::string::npos) {
				continue;
			}
			auto key = Lower(Trim(line.substr(0, eq)));
			auto value = Trim(line.substr(eq + 1));
			if (const auto c = value.find(';'); c != std::string::npos) {
				value = Trim(value.substr(0, c));
			}
			if (!key.empty()) {
				doc[section][key] = value;
			}
		}
		return doc;
	}

	inline std::optional<std::string> GetString(const Document& a_doc, const std::string& a_section, const std::string& a_key)
	{
		const auto s = a_doc.find(Lower(a_section));
		if (s == a_doc.end()) {
			return std::nullopt;
		}
		const auto k = s->second.find(Lower(a_key));
		if (k == s->second.end() || k->second.empty()) {
			return std::nullopt;
		}
		return k->second;
	}

	// Returns nullopt if the key is absent OR not a finite number.
	inline std::optional<float> GetFloat(const Document& a_doc, const std::string& a_section, const std::string& a_key)
	{
		const auto str = GetString(a_doc, a_section, a_key);
		if (!str) {
			return std::nullopt;
		}
		errno = 0;
		char* end = nullptr;
		const float v = std::strtof(str->c_str(), &end);
		if (end == str->c_str() || *end != '\0' || errno == ERANGE || !std::isfinite(v)) {
			return std::nullopt;
		}
		return v;
	}

	inline std::optional<bool> GetBool(const Document& a_doc, const std::string& a_section, const std::string& a_key)
	{
		const auto str = GetString(a_doc, a_section, a_key);
		if (!str) {
			return std::nullopt;
		}
		const auto v = Lower(*str);
		if (v == "1" || v == "true" || v == "yes" || v == "on") {
			return true;
		}
		if (v == "0" || v == "false" || v == "no" || v == "off") {
			return false;
		}
		return std::nullopt;
	}
}
