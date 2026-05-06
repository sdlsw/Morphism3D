#pragma once
#include "global_defines.h"

#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace g3d {
enum class CameraMode : int {
	forward = 0,
	fixedLook
};

constexpr unsigned int CameraModeCount = 2;

class Camera {
private:
	// Directional vectors. Automatically calculated from horizontal and
	// vertical angle. For this application, +Z is considered "up" due to
	// 3D graphing conventions.
	glm::vec3 _forward;
	glm::vec3 _up;
	glm::vec3 _right;

	void modAngles();
	void updateVectors();
public:
	static inline const float defaultProjectionMix = 1.0f;

	// Determines the mixture of perspective vs. orthographic projection.
	// 0 is entirely orthographic, 1 is entirely perspective. This setting
	// is ignored in forward mode.
	float projectionMix = defaultProjectionMix;

	// Rotation about the X and Z axes, respectively. Values in radians.
	glm::vec2 angles { 0.0f, 0.0f };

	// Camera position
	glm::vec3 position { 0.0f, 0.0f, 0.0f};

	// Camera is locked looking at this point if mode is fixedLook.
	glm::vec3 lookPosition { 0.0f, 0.0f, 0.0f };

	CameraMode mode = CameraMode::forward;

	Camera() { updateVectors(); }
	Camera(CameraMode _mode) : mode { _mode } { updateVectors(); }

	// Should be called every frame, once all modifications are done.
	void update();

	const glm::vec3& forward() const { return _forward; }
	const glm::vec3& up() const { return _up; }
	const glm::vec3& right() const { return _right; }

	// Sets forward vector directly by calculating horizontal and vertical
	// angles.
	void forward(const glm::vec3& fwd);

	// Sets forward vector directly to look at a specific position.
	void lookAt(const glm::vec3& pos);

	glm::mat4 viewMatrix() const;
	glm::mat4 projectionMatrix(unsigned int width, unsigned int height) const;
};
}
