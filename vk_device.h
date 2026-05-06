#pragma once
#include "global_defines.h"

// Top level Vulkan object wrappers, physical device/memory management.

#include "vk_concept.h"
#include "vk_helper.h"
#include "window.h"

#include <vulkan/vulkan_raii.hpp>

namespace g3d {
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() const;
};

struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

// Class wrapping the top level Vulkan objects to be used by the application.
class VkTop {
private:
	vk::ApplicationInfo _appInfo;
	vk::raii::Context _context;
	vk::raii::Instance _instance;
	vk::raii::PhysicalDevices _physicalDevices;

	vk::raii::Instance createInstance();
public:
	VkTop() = delete;
	VkTop(VkTop&&) = default;
	VkTop(const vk::ApplicationInfo& appInfo);
	const vk::ApplicationInfo& appInfo() const { return _appInfo; }
	vk::raii::Context& context() { return _context; }
	vk::raii::Instance& instance() { return _instance; }
	vk::raii::PhysicalDevices& physicalDevices() { return _physicalDevices; }
};

// Class wrapping a Vulkan logical device and the various top level objects
// required to construct it. Also holds references to the queues.
class GraphDevice {
private:
	VkTop* _vkTop;
	Window* _window;
	vk::raii::SurfaceKHR _surface; // NOTE: Allocated by GLFW, but freed by vulkan-hpp
	vk::raii::PhysicalDevice* _physicalDevice;
	QueueFamilyIndices _queueFamilyIndices;
	vk::raii::Device _device;
	vk::raii::Queue _graphicsQueue;
	vk::raii::Queue _presentQueue;
	vk::raii::CommandPool _commandPool;

	// Private init functions for constructor
	vk::raii::Device createLogicalDevice();
	vk::raii::CommandPool createCommandPool();

	// Query functions for determining physical device to use.
	QueueFamilyIndices queryQueueFamilies(const vk::raii::PhysicalDevice& device) const;
	SwapChainSupportDetails _querySwapChainSupport(const vk::raii::PhysicalDevice& device) const;
	bool pDevSupported(const vk::raii::PhysicalDevice& device) const;
	vk::raii::PhysicalDevice& pickPhysicalDevice(vk::raii::PhysicalDevices& devices);
public:
	GraphDevice() = delete;
	GraphDevice(GraphDevice&&) = default;

	GraphDevice(VkTop& vkTop, Window& window)
	: _vkTop { &vkTop },
	  _window { &window },
	  _surface { window.createSurface(vkTop.instance()) },
	  _physicalDevice { &pickPhysicalDevice(vkTop.physicalDevices()) },
	  _queueFamilyIndices { queryQueueFamilies(*_physicalDevice) },
	  _device { createLogicalDevice() },
	  _graphicsQueue { _device.getQueue(_queueFamilyIndices.graphicsFamily.value(), 0) },
	  _presentQueue { _device.getQueue(_queueFamilyIndices.presentFamily.value(), 0) },
	  _commandPool { createCommandPool() }
	{}

	auto& vkTop() { return *_vkTop; }
	auto& window() { return *_window; }
	auto& surface() { return _surface; }
	auto& logicalDevice() { return _device; }
	auto& physicalDevice() { return *_physicalDevice; }
	auto& graphicsQueue() { return _graphicsQueue; }
	auto& presentQueue() { return _presentQueue; }
	auto& commandPool() { return _commandPool; }

	auto querySwapChainSupport() const { return _querySwapChainSupport(*_physicalDevice); }

	bool queueFamiliesAreSame() const {
		return _queueFamilyIndices.graphicsFamily == _queueFamilyIndices.presentFamily;
	}
	const QueueFamilyIndices& queueFamilies() const { return _queueFamilyIndices; }
	std::array<uint32_t, 2> queueFamiliesAsArray() const {
		std::array<uint32_t, 2> a {
			_queueFamilyIndices.graphicsFamily.value(),
			_queueFamilyIndices.presentFamily.value()
		};
		return a;
	}

	vk::Format findSupportedFormat(
		const std::vector<vk::Format>& candidates,
		vk::ImageTiling tiling,
		vk::FormatFeatureFlags features
	);
	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
	vk::raii::DeviceMemory allocateRequiredMemory(
		const vk::MemoryRequirements& requirements,
		const vk::MemoryPropertyFlags& properties
	);

	// Create and bind a new memory allocation for an individual object.
	// This is very much not the recommended way to do this in Vulkan (the
	// preferred way is to allocate a large chunk of device memory and then
	// manage it yourself), but this program isn't going to have many
	// resources to allocate so it SHOULD be okay. TODO come back to this
	template<MemoryBindable T>
	vk::raii::DeviceMemory bindNewMemory(
		T& obj,
		vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal
	) {
		auto memory = allocateRequiredMemory(
			obj.getMemoryRequirements(),
			properties
		);
		obj.bindMemory(memory, 0);
		return memory;
	}

	OneTimeCommandBuffer beginOneTimeCommands();
};

class BoundBuffer {
protected:
	GraphDevice* _graphDevice;
	vk::BufferCreateInfo _info;
	vk::raii::Buffer _buffer;
	vk::raii::DeviceMemory _memory;
public:
	BoundBuffer() = delete;
	BoundBuffer(BoundBuffer&&) = default;
	BoundBuffer(
		GraphDevice& graphDevice,
		const vk::BufferCreateInfo& info,
		vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal
	)
	: _graphDevice { &graphDevice },
	  _info { info },
	  _buffer { _graphDevice->logicalDevice(), info },
	  _memory { _graphDevice->bindNewMemory(_buffer, properties) }
	{}

	operator vk::raii::Buffer&() { return _buffer; }
	auto& buffer() { return _buffer; }
	const auto& buffer() const { return _buffer; }
	const auto& info() { return _info; }
	vk::DescriptorBufferInfo descriptorInfo() const {
		vk::DescriptorBufferInfo info { .buffer = *_buffer, .offset = 0, .range = _info.size };
		return info;
	}

	void vkCopyTo(BoundBuffer& other);
};

class UnsafeMappedBuffer : public BoundBuffer {
private:
	static inline const vk::MemoryPropertyFlags MEMORY_PROPERTIES = (
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent
	);

protected:
	void* _ptr;

public:
	UnsafeMappedBuffer() = delete;
	UnsafeMappedBuffer(UnsafeMappedBuffer&&) = default;
	UnsafeMappedBuffer(
		GraphDevice& graphDevice,
		vk::DeviceSize size,
		vk::BufferUsageFlags usage,
		vk::SharingMode sharingMode = vk::SharingMode::eExclusive
	)
	: BoundBuffer(
		graphDevice,
		{ .size = size, .usage = usage, .sharingMode = sharingMode },
		MEMORY_PROPERTIES
	  ),
	  _ptr { _memory.mapMemory(0, size, {}) }
	{}

	auto unsafePtr() { return _ptr; }
	void unsafeCopyIn(const void* data, size_t size);
};

// Same as UnsafeMappedBuffer, but tied to a specific datatype. Unfortunately
// can't be used for dynamically sized things.
template<typename T>
class MappedBuffer : public UnsafeMappedBuffer {
public:
	MappedBuffer() = delete;
	MappedBuffer(MappedBuffer&&) = default;
	MappedBuffer(
		GraphDevice& graphDevice,
		vk::BufferUsageFlags usage,
		vk::SharingMode sharingMode = vk::SharingMode::eExclusive
	)
	: UnsafeMappedBuffer(graphDevice, sizeof(T), usage, sharingMode)
	{}

	operator T*() { return reinterpret_cast<T*>(_ptr); }
	auto ptr() { return reinterpret_cast<T*>(_ptr); }
	void copyIn(const T& obj) {
		memcpy(_ptr, &obj, sizeof(T));
	}
};

}
