#include "logging.h"

#include <exception>
#include <unordered_map>
#include <utility>

static const g3d::LogLevel defaultLevel = g3d::LogLevel::Debug;

static g3d::LogLevel globalLevel = defaultLevel;
static std::unordered_map<std::string, g3d::LogLevel> localLevels;

static const std::string levelNames[] {
	"DISABLED",
	"ERROR",
	"WARNING",
	"INFO",
	"DEBUG",
	"FINE"
};

namespace g3d {
const std::string& stringFromLogLevel(LogLevel level) {
	return levelNames[std::to_underlying(level)];
}

LogLevel globalLogLevel() {
	return globalLevel;
}

void globalLogLevel(LogLevel level) {
	globalLevel = level;
}

LogLevel localLogLevel(const std::string& name) {
	if (!localLevels.contains(name)) {
		return globalLevel;
	}

	return localLevels.at(name);
}

void overrideLocalLogLevel(const std::string& name, LogLevel level) {
	localLevels[name] = level;
}

void clearLocalLogLevel(const std::string& name) {
	localLevels.erase(name);
}
}
