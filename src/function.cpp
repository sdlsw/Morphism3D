#include "function.h"

static g3d::Logger _logger { "function" };

namespace g3d {
namespace function::detail {
Logger& logger() { return _logger; }
}
}
