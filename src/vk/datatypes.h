#pragma once
#include "global_defines.h"

// Simple data containers that don't directly depend on Vulkan.

#include <vulkan/vulkan.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <array>

namespace g3d {
// Shader input types. Each item is passed in as its own buffer, nothing is
// interleaved.
struct Position {
	static constexpr unsigned int binding = 0;
	glm::vec3 vec;
	constexpr Position(const glm::vec3& v) : vec {v} {}
	constexpr Position(float x, float y, float z) : vec {x, y, z} {}
};

struct Color {
	static constexpr unsigned int binding = 1;
	glm::vec3 vec;
	constexpr Color(const glm::vec3& v) : vec {v} {};
	constexpr Color(float r, float g, float b) : vec {r, g, b} {}
};

struct Normal {
	static constexpr unsigned int binding = 2;
	glm::vec3 vec;
	constexpr Normal(const glm::vec3& v) : vec {v} {};
	constexpr Normal(float x, float y, float z) : vec {x, y, z} {}
};

template<typename T>
vk::VertexInputBindingDescription getVertexBindingDescription() {
	vk::VertexInputBindingDescription d {
		.binding = T::binding,
		.stride = sizeof(T),
		.inputRate = vk::VertexInputRate::eVertex
	};
	return d;
}

template <typename T>
vk::VertexInputAttributeDescription getVertexAttributeDescription() {
	vk::VertexInputAttributeDescription d {
		.location = T::binding,
		.binding = T::binding,
		.format = vk::Format::eR32G32B32Sfloat,
		.offset = offsetof(Position, vec)
	};
	return d;
}

struct Transform {
	// For use in constructors
	static inline const glm::vec3 default_scale { 1.0f, 1.0f, 1.0f };

	glm::vec3 translation { 0.0f, 0.0f, 0.0f};
	glm::vec3 scale { 1.0f, 1.0f, 1.0f };
	glm::mat4 rotation { 1.0f };

	Transform() = default;
	Transform(const glm::vec3& t) : translation {t} {}
	Transform(const glm::vec3& t, const glm::vec3& s) : translation {t}, scale {s} {}
	Transform(const glm::vec3& t, const glm::vec3& s, const glm::mat4& r) : translation {t}, scale {s}, rotation {r} {}

	glm::mat4 matrix() const;
};

struct Light {
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 color;
	alignas(4) float mix;
};

struct Material {
	alignas(16) glm::vec3 ambient;
	alignas(16) glm::vec3 diffuse;
	alignas(16) glm::vec3 specular;
	float shine;

	Material() = default;
	Material(
		const glm::vec3& _ambient,
		const glm::vec3& _diffuse,
		const glm::vec3& _specular,
		float _shine
	)
	: ambient { _ambient },
	  diffuse { _diffuse },
	  specular { _specular },
	  shine { _shine }
	{}
	Material(const glm::vec4& strengths)
	: ambient { strengths.x },
	  diffuse { strengths.y },
	  specular { strengths.z },
	  shine { strengths.w }
	{}
	Material(float a, float d, float s, float shine)
	: ambient { a }, diffuse { d }, specular { s }, shine { shine }
	{}
};
}
