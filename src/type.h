#include <string>
#include <typeinfo>

namespace g3d {
std::string demangle(const char* name);

template<typename E>
std::string prettyType() {
	return demangle(typeid(E).name());
}
}
