#include "vk/device.h"

#include <set>
#include <string>

#ifndef NDEBUG
static const bool ENABLE_VALIDATION_LAYERS = false;
#else
static const bool ENABLE_VALIDATION_LAYERS = true;
#endif

static const std::vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_KHRONOS_validation"
};

static const std::vector<const char*> REQUIRED_DEVICE_EXTENSIONS = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Does this physical device support all the extensions we need?
static bool pdevHasRequiredExtensions(const vk::raii::PhysicalDevice& device) {
	std::set<std::string> extSet(REQUIRED_DEVICE_EXTENSIONS.begin(), REQUIRED_DEVICE_EXTENSIONS.end());
	for (const auto& ext : device.enumerateDeviceExtensionProperties()) {
		extSet.erase(ext.extensionName);
	}

	bool hasRequired = extSet.empty();

	if (!hasRequired) {
		for (const auto& extName : extSet) {
			std::cerr << "missing required extension \"" << extName << "\"" << std::endl;
		}
	}

	return hasRequired;
}

namespace g3d {
bool QueueFamilyIndices::isComplete() const {
	return graphicsFamily.has_value() && presentFamily.has_value();
}

VkTop::VkTop(const vk::ApplicationInfo& appInfo) 
: _appInfo {appInfo},
  _instance { createInstance() },
  _physicalDevices { _instance }
{
	size_t dev_count = _physicalDevices.size();

	if (dev_count != 0) {
		std::cerr << "enumerated " << dev_count << " device(s) with Vulkan support" << std::endl;
	} else {
		throw std::runtime_error("failed to find a device with Vulkan support!");
	}
}

vk::raii::Instance VkTop::createInstance() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	if (glfwExtensionCount != 0) {
		std::cerr << "glfwExtensions:" << std::endl;
		for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
			std::cerr << '\t' << glfwExtensions[i] << std::endl;
		}
	} else {
		std::cerr << "GLFW reported no required extensions" << std::endl;
	}

	vk::InstanceCreateInfo info {
		.pApplicationInfo = &_appInfo,
		.enabledExtensionCount = glfwExtensionCount,
		.ppEnabledExtensionNames = glfwExtensions
	};

	if (ENABLE_VALIDATION_LAYERS) {
		info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}

	return vk::raii::Instance(_context, info);
}

QueueFamilyIndices GraphDevice::queryQueueFamilies(const vk::raii::PhysicalDevice& device) const {
	QueueFamilyIndices indices;

	const std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
	for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
		const auto& familyProperties = queueFamilies[i];
		if (familyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = i;
		}

		vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, _surface);
		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) break;
	}

	return indices;
}

SwapChainSupportDetails GraphDevice::_querySwapChainSupport(const vk::raii::PhysicalDevice& device) const {
	SwapChainSupportDetails details {
		device.getSurfaceCapabilitiesKHR(_surface),
		device.getSurfaceFormatsKHR(_surface),
		device.getSurfacePresentModesKHR(_surface)
	};

	return details;
}

// Is this physical device supported by our application?
// (For now, just pick the first one that meets every need. TODO May come back to
// this.)
bool GraphDevice::pDevSupported(const vk::raii::PhysicalDevice& device) const {
	if (!queryQueueFamilies(device).isComplete()) {
		std::cerr << "device does not support required queue types" << std::endl;
		return false;
	}

	// FIXME: checking for surface support after creating surface...
	if (!pdevHasRequiredExtensions(device)) {
		std::cerr << "device does not support required extensions" << std::endl;
		return false;
	}

	SwapChainSupportDetails scSupportDetails = _querySwapChainSupport(device);
	if (scSupportDetails.formats.empty()) {
		std::cerr << "window surface supports no formats" << std::endl;
		return false;
	}
	if (scSupportDetails.presentModes.empty()) {
		std::cerr << "window surface supports no presentation modes" << std::endl;
		return false;
	}

	return true;
}

vk::raii::PhysicalDevice& GraphDevice::pickPhysicalDevice(vk::raii::PhysicalDevices& devices) {
	if (devices.size() == 0) {
		throw std::invalid_argument("cannot pick physical device from empty list");
	}

	for (auto& device : devices) {
		if (pDevSupported(device)) {
			std::cerr << "found suitable device: " << device.getProperties().deviceName << std::endl;
			return device;
		}
	}

	throw std::runtime_error("no enumerated device supports required features!");
}

vk::raii::Device GraphDevice::createLogicalDevice() {
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies {
		_queueFamilyIndices.graphicsFamily.value(),
		_queueFamilyIndices.presentFamily.value()
	};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		queueCreateInfos.push_back({
			.queueFamilyIndex = queueFamily,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority
		});
	}

	vk::PhysicalDeviceFeatures deviceFeatures {};

	vk::DeviceCreateInfo createInfo {
		.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(REQUIRED_DEVICE_EXTENSIONS.size()),
		.ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data(),
		.pEnabledFeatures = &deviceFeatures
	};

	if (ENABLE_VALIDATION_LAYERS) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}

	return _physicalDevice->createDevice(createInfo, nullptr);
}

vk::raii::CommandPool GraphDevice::createCommandPool() {
	vk::CommandPoolCreateInfo info {
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = _queueFamilyIndices.graphicsFamily.value()
	};

	return vk::raii::CommandPool(_device, info);
}

OneTimeCommandBuffer GraphDevice::beginOneTimeCommands() {
	return OneTimeCommandBuffer(_device, _commandPool, _graphicsQueue);
}

vk::Format GraphDevice::findSupportedFormat(
	const std::vector<vk::Format>& candidates,
	vk::ImageTiling tiling,
	vk::FormatFeatureFlags features
) {
	for (vk::Format format : candidates) {
		vk::FormatProperties props = _physicalDevice->getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format");
}

uint32_t GraphDevice::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
	vk::PhysicalDeviceMemoryProperties memProperties = _physicalDevice->getMemoryProperties();
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type");
}

vk::raii::DeviceMemory GraphDevice::allocateRequiredMemory(
	const vk::MemoryRequirements& requirements,
	const vk::MemoryPropertyFlags& properties
) {
	vk::MemoryAllocateInfo info {
		.allocationSize = requirements.size,
		.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, properties)
	};
	return vk::raii::DeviceMemory(_device, info);
}

void BoundBuffer::vkCopyTo(BoundBuffer& other) {
	if (other.info().size != _info.size) {
		throw std::invalid_argument("vkCopyTo: mismatched buffer sizes");
	}

	OneTimeCommandBuffer b = _graphDevice->beginOneTimeCommands();

	vk::BufferCopy copyRegion {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = _info.size
	};
	b.buffer().copyBuffer(_buffer, other.buffer(), copyRegion);
}

void UnsafeMappedBuffer::unsafeCopyIn(const void* obj, size_t size) {
	memcpy(_ptr, obj, size);
}
}
