#include "event.h"

static g3d::Logger _logger { "event" };

namespace g3d {
namespace event::detail {
Logger& logger() { return _logger; }
}

void EventRouter::deleteLink(uint64_t id) {
	if (!_idmap.contains(id)) return;
	auto& [ptr, type_index] = _idmap.at(id);
	_typemap[type_index].erase(id);
	_idmap.erase(id);
}

void EventRouter::updateLink(uint64_t id, Linked<EventRouter>* newPtr) {
	if (!_idmap.contains(id)) return;

	_logger.debug(
		"update link {} -> {}",
		reinterpret_cast<void*>(_idmap.at(id).first),
		reinterpret_cast<void*>(newPtr)
	);
	auto type_index = _idmap.at(id).second;

	_typemap[type_index][id] = newPtr;

	_idmap.erase(id);
	auto p = std::pair<Linked<EventRouter>*, std::type_index>(newPtr, type_index);
	_idmap.emplace(id, std::move(p));
}
}
