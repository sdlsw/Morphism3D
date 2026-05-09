#include "vk_datatypes.h"

#include <glm/gtc/matrix_transform.hpp>

namespace g3d {
glm::mat4 Transform::matrix() const {
	glm::mat4 m { 1.0f };
	m = glm::translate(m, translation);
	m = glm::scale(m, scale);
	return m * rotation;
}
}
