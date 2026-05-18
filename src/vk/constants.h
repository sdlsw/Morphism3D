#pragma once

#include <vulkan/vulkan.hpp>

// Used mostly for debugging rendering issues, should be left defined normally
#define MSAA_ENABLE

namespace g3d {
	constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

#ifdef MSAA_ENABLE
	constexpr vk::SampleCountFlagBits MSAA_SAMPLES = vk::SampleCountFlagBits::e4;
#else
	constexpr vk::SampleCountFlagBits MSAA_SAMPLES = vk::SampleCountFlagBits::e1;
#endif
}
