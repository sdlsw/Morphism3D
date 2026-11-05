#pragma once

#include <algorithm>
#include <cstdint>
#include <format>
#include <iostream>
#include <string>

namespace g3d {
// Ordered from least detail to most detail.
enum class LogLevel : uint8_t {
	Disabled = 0,
	Error,
	Warning,
	Info,
	Debug,
	Fine
};

const std::string& stringFromLogLevel(LogLevel level);

LogLevel globalLogLevel();
void globalLogLevel(LogLevel level);

LogLevel localLogLevel(const std::string& name);
void overrideLocalLogLevel(const std::string& name, LogLevel level);
void clearLocalLogLevel(const std::string& name);

template<typename... Args>
void logToName(
	const std::string& loggerName,
	LogLevel level,
	std::format_string<Args...> format,
	Args&&... args
) {
	if (level <= localLogLevel(loggerName)) {
		std::cerr << '[' << stringFromLogLevel(level) << "] "
			<< loggerName << ": "
			<< std::format(format, std::forward<Args>(args)...)
			<< std::endl;
	}
}

struct Logger {
	std::string name;

	LogLevel level() const;
	void overrideLevel(LogLevel level);
	void clearOverride();

	Logger(const std::string& name, LogLevel _level) : name { name } {
		overrideLevel(_level);
	}

	Logger(const std::string& name) : name { name } {}

	template<typename... Args>
	void log(LogLevel level, std::format_string<Args...> format, Args&&... args) const {
		logToName(name, level, format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void error(std::format_string<Args...> format, Args&&... args) const {
		log(LogLevel::Error, format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void warning(std::format_string<Args...> format, Args&&... args) const {
		log(LogLevel::Warning, format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void info(std::format_string<Args...> format, Args&&... args) const {
		log(LogLevel::Info, format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void debug(std::format_string<Args...> format, Args&&... args) const {
		log(LogLevel::Debug, format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void fine(std::format_string<Args...> format, Args&&... args) const {
		log(LogLevel::Fine, format, std::forward<Args>(args)...);
	}
};
}
