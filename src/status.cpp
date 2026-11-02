#include "status.h"

static const std::string empty {""};

namespace g3d {
StatusMessageEvent StatusMessageEvent::clear() {
	return { empty, 0.0f };
}
}
