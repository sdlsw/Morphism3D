#include "type.h"
#ifdef __GNUG__
#include <cxxabi.h>
#include <memory>
#endif

struct StrHandle {
	char* s;
	StrHandle(char* s) : s { s } {}
	~StrHandle() { if (s != nullptr) std::free(s); }
};

namespace g3d {
#ifdef __GNUG__
std::string demangle(const char* name) {
	int status = -4;
	StrHandle handle { abi::__cxa_demangle(name, nullptr, nullptr, &status) };

	if (handle.s != nullptr) {
		return { handle.s };
	}

	return { "!!DEMANGLE_FAILED!!" };
}
#else
std::string demangle(const char* name) {
	return { name };
}
#endif
}
