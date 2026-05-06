#pragma once
#include "global_defines.h"

// Simple data containers that don't directly depend on Vulkan.

#include <vulkan/vulkan.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <array>

namespace g3d {
struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	static vk::VertexInputBindingDescription getBindingDescription();
	static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions();
};

struct Transform {
	// For use in constructors
	static inline const glm::vec3 default_scale { 1.0f, 1.0f, 1.0f };

	glm::vec3 translation { 0.0f, 0.0f, 0.0f};
	glm::vec3 scale { 1.0f, 1.0f, 1.0f };
	glm::mat4 rotation { 1.0f };

	Transform() = default;
	Transform(const glm::vec3& t) : translation {t} {}
	Transform(const glm::vec3& t, const glm::vec3& s) : translation {t}, scale {s} {}
	Transform(const glm::vec3& t, const glm::vec3& s, const glm::mat4 r) : translation {t}, scale {s}, rotation {r} {}

	glm::mat4 matrix() const;
};
}
