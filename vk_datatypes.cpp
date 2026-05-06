#include "vk_datatypes.h"

#include <glm/gtc/matrix_transform.hpp>

namespace g3d {
vk::VertexInputBindingDescription Vertex::getBindingDescription() {
	vk::VertexInputBindingDescription d {
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = vk::VertexInputRate::eVertex
	};
	return d;
}

std::array<vk::VertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions() {
	std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions {};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	return attributeDescriptions;
}

glm::mat4 Transform::matrix() const {
	glm::mat4 m { 1.0f };
	m = glm::translate(m, translation);
	m = glm::scale(m, scale);
	return m * rotation;
}
}
