#include "event.h"

namespace g3d {
void EventRouter::deleteLink(uint64_t id) {
	if (!_idmap.contains(id)) return;
	auto& [ptr, type_index] = _idmap.at(id);
	_typemap[type_index].erase(id);
	_idmap.erase(id);
}
}
