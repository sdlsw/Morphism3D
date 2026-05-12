#include "vk/camera.h"

#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace g3d {
void Camera::modAngles() {
	auto fullCircle = 2.0f * glm::pi<float>();

	if (angles.x > fullCircle || angles.x < 0) {
		angles.x = glm::mod(angles.x, fullCircle);
	}

	if (angles.y > fullCircle || angles.y < 0) {
		angles.y = glm::mod(angles.y, fullCircle);
	}
}

void Camera::updateVectors() {
	glm::mat4 hrot = glm::rotate(glm::mat4(1.0f), angles.x, {0.0f, 0.0f, 1.0f});
	_right = glm::vec3(hrot * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

	glm::mat4 vrot = glm::rotate(glm::mat4(1.0f), angles.y, _right);
	_forward = glm::vec3(vrot * hrot * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
	_up = glm::vec3(vrot * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
}

void Camera::update() {
	modAngles();
	updateVectors();

	// In fixedLook mode, position is automatically updated based on angles
	if (mode == CameraMode::fixedLook) {
		position = lookPosition - glm::distance(lookPosition, position)*_forward;
	}
}

void Camera::forward(const glm::vec3& fwd) {
	angles.x = glm::acos(fwd.y / glm::length(glm::vec2(fwd.x, fwd.y)));
	if (fwd.x > 0.0f) angles.x = 2.0f*glm::pi<float>() - angles.x;

	glm::mat4 hrot = glm::rotate(glm::mat4(1.0f), angles.x, {0.0f, 0.0f, 1.0f});
	glm::vec3 right = glm::vec3(hrot * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

	angles.y = glm::orientedAngle(
		glm::normalize(glm::vec3(fwd.x, fwd.y, 0.0f)),
		glm::normalize(fwd),
		right
	);
}

void Camera::lookAt(const glm::vec3& pos) {
	forward(pos - position);
}

glm::mat4 Camera::viewMatrix() const {
	if (mode == CameraMode::forward) {
		return glm::lookAt(position, position + _forward, _up);
	} else {
		return glm::lookAt(position, lookPosition, _up);
	}
}

glm::mat4 Camera::projectionMatrix(unsigned int width, unsigned int height) const {
	float fheight = static_cast<float>(height);
	float fwidth = static_cast<float>(width);
	float aspect = fwidth / fheight;

	glm::mat4 perspective = glm::perspective(
		glm::radians(45.0f),
		aspect,
		0.1f,
		100.0f
	);

	// compensate for Vulkan inverted Y clip coord
	perspective[1][1] *= -1;

	// In forward mode, ignore orthographic setting, since it doesn't
	// make sense.
	if (mode == CameraMode::forward) {
		return std::move(perspective);
	}

	// Scale the orthographic prism's horizontal and vertical sizes
	// relative to distance to lookAt point, sort of faking a perspective
	// divide. This allows moving the camera towards and away from the
	// center to zoom in and out, as it does in pure perspective
	// projection, making the transition between perspective and
	// orthographic much smoother.
	float orthoScale = glm::distance(position, lookPosition) / 2.0f;
	glm::mat4 ortho = glm::ortho(
		-aspect * orthoScale, aspect * orthoScale,
		-1.0f * orthoScale, 1.0f * orthoScale,
		-100.0f,
		100.0f
	);
	ortho[1][1] *= -1;

	// glm doesn't support mixing matrices, so have to do it myself here
	glm::mat4 mixed = ortho * (1.0f - projectionMix) + perspective * projectionMix;

	return std::move(mixed);
}
}
