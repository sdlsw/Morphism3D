#include "container.h"

namespace g3d {
const std::string& DiscriminatedStringMap::operator[](const std::string& key) {
	if (!_cache.contains(key)) {
		_cache.emplace(key, std::format("{}{}", key, _suffix));
	}

	return _cache.at(key);
}
}
