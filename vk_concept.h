#pragma once

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <concepts>

namespace g3d {
template<typename T>
concept MemoryBindable = requires(T obj, vk::raii::DeviceMemory m, int n) {
	{ obj.getMemoryRequirements() } -> std::convertible_to<vk::MemoryRequirements>;
	{ obj.bindMemory(m, n) };
};
}
