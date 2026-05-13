#pragma once

#include <vulkan/vulkan.hpp>

namespace g3d {
	constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	constexpr vk::SampleCountFlagBits MSAA_SAMPLES = vk::SampleCountFlagBits::e4;
}
